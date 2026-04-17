/*
 * Happy-path PPP/IP/TCP stack for UNIVAC
 *
 * Runs on the UNIVAC emulator, communicates via serial channel 10.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syscalls.h>

#define IP(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define SERIAL_CHANNEL 7

/* === Raw I/O (bypass stdio remapping) === */

static inline void raw_putc(uint8_t b) {
  SYSCALL2(SYS_PUTC, b, SERIAL_CHANNEL);
}

static inline int raw_getc(void) { return SYSCALL(SYS_GETC, SERIAL_CHANNEL); }

void put(uint8_t b) { raw_putc(b); }

/* === PPP === */

uint16_t ppp_fcs(uint8_t *data, int len) {
  uint16_t fcs = 0xFFFF;
  for (int i = 0; i < len; i++) {
    fcs ^= data[i];
    for (int j = 0; j < 8; j++)
      fcs = (fcs & 1) ? (fcs >> 1) ^ 0x8408 : fcs >> 1;
  }
  return fcs ^ 0xFFFF;
}

void ppp_send(uint16_t proto, uint8_t *payload, int len) {
  uint8_t buf[2048];
  buf[0] = 0xFF;
  buf[1] = 0x03;
  buf[2] = proto >> 8;
  buf[3] = proto & 0xFF;
  memcpy(buf + 4, payload, len);
  uint16_t fcs = ppp_fcs(buf, len + 4);
  buf[len + 4] = fcs & 0xFF;
  buf[len + 5] = fcs >> 8;

  put(0x7E);
  for (int i = 0; i < len + 6; i++) {
    uint8_t b = buf[i];
    if (b < 0x20 || b == 0x7D || b == 0x7E) {
      put(0x7D);
      put(b ^ 0x20);
    } else {
      put(b);
    }
  }
  put(0x7E);
}

int ppp_recv(uint16_t *proto, uint8_t *payload) {
  static int have_flag = 0;
  int c;
  uint8_t buf[2048];
  int len, esc;

  for (;;) {
    if (!have_flag) {
      while ((c = raw_getc()) >= 0 && c != 0x7E)
        ;
      if (c < 0)
        return -1;
    }
    have_flag = 0;

    len = 0;
    esc = 0;
    while ((c = raw_getc()) >= 0 && c != 0x7E) {
      if (esc) {
        buf[len++] = c ^ 0x20;
        esc = 0;
      } else if (c == 0x7D)
        esc = 1;
      else
        buf[len++] = c;
    }

    if (c < 0)
      return -1;
    have_flag = 1; /* consumed 0x7E is opening flag of next frame */

    if (len >= 6)
      break; /* got a real frame */
    /* short/empty frame (inter-frame fill) — retry */
  }

  *proto = (buf[2] << 8) | buf[3];
  memcpy(payload, buf + 4, len - 6);
  return len - 6;
}

/* === Checksum === */

uint16_t checksum(uint8_t *data, int len) {
  uint32_t sum = 0;
  for (int i = 0; i < len - 1; i += 2)
    sum += (data[i] << 8) | data[i + 1];
  if (len & 1)
    sum += data[len - 1] << 8;
  while (sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);
  return ~sum & 0xFFFF;
}

/* === Packets === */

void ip_packet(uint8_t *pkt, int *len, uint32_t src, uint32_t dst,
               uint8_t *payload, int plen) {
  uint8_t hdr[20] = {0x45, 0, 0, 0, 0, 0, 0x40, 0, 64, 6, 0, 0};
  int total = 20 + plen;
  hdr[2] = total >> 8;
  hdr[3] = total & 0xFF;
  memcpy(hdr + 12, &src, 4);
  memcpy(hdr + 16, &dst, 4);
  uint16_t csum = checksum(hdr, 20);
  hdr[10] = csum >> 8;
  hdr[11] = csum & 0xFF;
  memcpy(pkt, hdr, 20);
  memcpy(pkt + 20, payload, plen);
  *len = total;
}

void tcp_packet(uint8_t *pkt, int *len, uint16_t sp, uint16_t dp, uint32_t seq,
                uint32_t ack, uint8_t flags, uint8_t *data, int dlen,
                uint32_t src, uint32_t dst) {
  uint8_t seg[2048] = {0};
  seg[0] = sp >> 8;
  seg[1] = sp & 0xFF;
  seg[2] = dp >> 8;
  seg[3] = dp & 0xFF;
  seg[4] = seq >> 24;
  seg[5] = seq >> 16;
  seg[6] = seq >> 8;
  seg[7] = seq & 0xFF;
  seg[8] = ack >> 24;
  seg[9] = ack >> 16;
  seg[10] = ack >> 8;
  seg[11] = ack & 0xFF;
  seg[12] = 0x50;
  seg[13] = flags;
  seg[14] = 0xFF;
  seg[15] = 0xFF; /* window */
  memcpy(seg + 20, data, dlen);

  /* pseudo header + segment for checksum */
  uint8_t pseudo[2048];
  memcpy(pseudo, &src, 4);
  memcpy(pseudo + 4, &dst, 4);
  pseudo[8] = 0;
  pseudo[9] = 6;
  int tlen = 20 + dlen;
  pseudo[10] = tlen >> 8;
  pseudo[11] = tlen & 0xFF;
  memcpy(pseudo + 12, seg, tlen);
  uint16_t csum = checksum(pseudo, 12 + tlen);
  seg[16] = csum >> 8;
  seg[17] = csum & 0xFF;

  memcpy(pkt, seg, tlen);
  *len = tlen;
}

/* === Main === */

int main() {
  uint32_t my_ip = IP(10, 0, 0, 2);
  uint16_t proto;
  uint8_t buf[2048], pkt[2048];
  int len, plen;
  int ipcp_done = 0, ipcp_id = 1;

/* Connection state: track a few recent connections */
#define MAX_CONNS 32
  static struct {
    uint16_t port;
    uint32_t seq;
    uint32_t ack_seq;
  } conns[MAX_CONNS];
  static int num_conns = 0;

  /* Send LCP Config-Request */
  buf[0] = 1;
  buf[1] = 1;
  buf[2] = 0;
  buf[3] = 4;
  ppp_send(0xC021, buf, 4);

  while ((len = ppp_recv(&proto, buf)) >= 0) {
    if (len == 0)
      continue; /* skip empty frames */
    /* LCP Config-Request -> ACK */
    if (proto == 0xC021 && buf[0] == 1) {
      buf[0] = 2; /* change to ACK */
      ppp_send(0xC021, buf, (buf[2] << 8) | buf[3]);
    }
    /* IPCP */
    else if (proto == 0x8021 && len >= 4) {
      int code = buf[0], pktlen = (buf[2] << 8) | buf[3];
      if (code == 1) { /* Config-Request -> ACK */
        buf[0] = 2;
        ppp_send(0x8021, buf, pktlen);
        if (!ipcp_done) { /* Send our request */
          uint8_t req[10] = {1, 0, 0, 10, 3, 6};
          req[1] = ipcp_id++;
          memcpy(req + 6, &my_ip, 4);
          ppp_send(0x8021, req, 10);
        }
      } else if (code == 2) { /* ACK */
        ipcp_done = 1;
      } else if (code == 3) { /* NAK - retry */
        uint8_t req[10] = {1, 0, 0, 10, 3, 6};
        req[1] = ipcp_id++;
        memcpy(req + 6, &my_ip, 4);
        ppp_send(0x8021, req, 10);
      }
    }
    /* IP */
    else if (proto == 0x0021 && len >= 40) {
      int ihl = (buf[0] & 0xF) * 4;
      uint32_t src_ip;
      memcpy(&src_ip, buf + 12, 4);
      if (buf[9] != 6)
        continue; /* TCP only */

      uint8_t *tcp = buf + ihl;
      uint16_t sp = (tcp[0] << 8) | tcp[1];
      uint32_t tseq = (tcp[4] << 24) | (tcp[5] << 16) | (tcp[6] << 8) | tcp[7];
      uint8_t flags = tcp[13];
      int doff = (tcp[12] >> 4) * 4;
      int dlen = len - ihl - doff;
      /* Find or create connection entry */
      int ci = -1;
      for (int i = 0; i < num_conns; i++) {
        if (conns[i].port == sp) {
          ci = i;
          break;
        }
      }

      if (flags & 0x02) { /* SYN -> SYN-ACK */
        if (ci >= 0) {
          /* Duplicate SYN for existing connection — skip */
        } else {
          if (num_conns < MAX_CONNS) {
            ci = num_conns++;
            conns[ci].port = sp;
          }
          if (ci >= 0) {
            conns[ci].seq = 1001;
            conns[ci].ack_seq = tseq + 1;
          }
          tcp_packet(pkt, &plen, 80, sp, 1000, tseq + 1, 0x12, NULL, 0, my_ip,
                     src_ip);
          ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
          ppp_send(0x0021, buf, len);
        }
      } else if (dlen > 0) { /* Data -> HTTP response */
        if (ci >= 0 && tseq < conns[ci].ack_seq) {
          /* Retransmitted data — re-ACK and skip */
          tcp_packet(pkt, &plen, 80, sp, conns[ci].seq, conns[ci].ack_seq,
                     0x10, NULL, 0, my_ip, src_ip);
          ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
          ppp_send(0x0021, buf, len);
          continue;
        }
        uint32_t seq = (ci >= 0) ? conns[ci].seq : 1001;
        if (ci >= 0)
          conns[ci].ack_seq = tseq + dlen;
        char *http = "HTTP/1.0 200 OK\r\nConnection: close\r\n"
                     "Content-Type: text/html\r\n"
                     "Content-Length: 76\r\n\r\n"
                     "<html><body><marquee>Hello from UNIVAC PPP/IP/TCP!</marquee></body></html>\n";
        int hlen = strlen(http);
        tcp_packet(pkt, &plen, 80, sp, seq, tseq + dlen, 0x18, (uint8_t *)http,
                   hlen, my_ip, src_ip);
        ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
        ppp_send(0x0021, buf, len);
        if (ci >= 0)
          conns[ci].seq = seq + hlen;
      } else if (flags & 0x01) { /* FIN -> FIN-ACK */
        uint32_t seq = (ci >= 0) ? conns[ci].seq : 1001;
        tcp_packet(pkt, &plen, 80, sp, seq, tseq + 1, 0x11, NULL, 0, my_ip,
                   src_ip);
        ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
        ppp_send(0x0021, buf, len);
        /* Remove connection */
        if (ci >= 0) {
          conns[ci] = conns[--num_conns];
        }
      }
    }
  }
  return 0;
}
