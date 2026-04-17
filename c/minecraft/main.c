/*
 * Happy-path PPP/IP/TCP + Minecraft server for UNIVAC
 *
 * Runs on the UNIVAC emulator, communicates via serial channel 7
 * Minecraft client connects to 10.0.0.2:25565
 */
#include <stdint.h>
#include <string.h>
#include <syscalls.h>

#define IP(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define SERIAL_CHANNEL 7
#define MC_PORT 25565

/* Minecraft connection states */
#define STATE_HANDSHAKE 0
#define STATE_LOGIN 2
#define STATE_CONFIG 3
#define STATE_PLAY 4

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
  uint8_t buf[1500];
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
  int c;
  while ((c = raw_getc()) >= 0 && c != 0x7E)
    ;
  if (c < 0)
    return -1;

  uint8_t buf[1500];
  int len = 0, esc = 0;
  while ((c = raw_getc()) >= 0 && c != 0x7E) {
    if (esc) {
      buf[len++] = c ^ 0x20;
      esc = 0;
    } else if (c == 0x7D)
      esc = 1;
    else if (len < (int)sizeof(buf))
      buf[len++] = c;
  }
  if (len < 6)
    return 0; /* empty/short frame, continue */

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
  uint8_t seg[1500] = {0};
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
  uint8_t pseudo[1500];
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

/* === Minecraft VarInt === */

int varint_decode(uint8_t *buf, int len, int32_t *out) {
  *out = 0;
  int shift = 0, i = 0;
  while (i < len && i < 5) {
    *out |= (buf[i] & 0x7F) << shift;
    if (!(buf[i] & 0x80))
      return i + 1;
    shift += 7;
    i++;
  }
  return -1;
}

int varint_encode(int32_t val, uint8_t *buf) {
  uint32_t uval = (uint32_t)val;
  int i = 0;
  do {
    buf[i] = uval & 0x7F;
    uval >>= 7;
    if (uval)
      buf[i] |= 0x80;
    i++;
  } while (uval);
  return i;
}

int varint_size(int32_t val) {
  uint32_t uval = (uint32_t)val;
  int s = 0;
  do {
    uval >>= 7;
    s++;
  } while (uval);
  return s;
}

/* === Minecraft Packet Helpers === */

int mc_read_string(uint8_t *buf, int len, char *out, int maxlen) {
  int32_t slen;
  int vi = varint_decode(buf, len, &slen);
  if (vi < 0 || slen > maxlen - 1 || vi + slen > len)
    return -1;
  memcpy(out, buf + vi, slen);
  out[slen] = 0;
  return vi + slen;
}

/* Build a Minecraft packet: length prefix + packet_id + data */
int mc_packet(uint8_t *out, int packet_id, uint8_t *data, int dlen) {
  int id_len = varint_size(packet_id);
  int pkt_len = id_len + dlen;
  int hdr_len = varint_encode(pkt_len, out);
  hdr_len += varint_encode(packet_id, out + hdr_len);
  if (data && dlen > 0)
    memcpy(out + hdr_len, data, dlen);
  return hdr_len + dlen;
}

/* === Registry Data (minimal for 1.21.8) === */

static const uint8_t known_packs[] = {0x18, 0x0e, 0x01, 0x09, 0x6d, 0x69, 0x6e,
                                      0x65, 0x63, 0x72, 0x61, 0x66, 0x74, 0x04,
                                      0x63, 0x6f, 0x72, 0x65, 0x06, 0x31, 0x2e,
                                      0x32, 0x31, 0x2e, 0x38};

static const uint8_t registries_bin[] = {
    0x13, 0x07, 0x0b, 0x63, 0x61, 0x74, 0x5f, 0x76, 0x61, 0x72, 0x69, 0x61,
    0x6e, 0x74, 0x01, 0x03, 0x72, 0x65, 0x64, 0x00, 0x1d, 0x07, 0x0f, 0x63,
    0x68, 0x69, 0x63, 0x6b, 0x65, 0x6e, 0x5f, 0x76, 0x61, 0x72, 0x69, 0x61,
    0x6e, 0x74, 0x01, 0x09, 0x74, 0x65, 0x6d, 0x70, 0x65, 0x72, 0x61, 0x74,
    0x65, 0x00, 0x19, 0x07, 0x0b, 0x63, 0x6f, 0x77, 0x5f, 0x76, 0x61, 0x72,
    0x69, 0x61, 0x6e, 0x74, 0x01, 0x09, 0x74, 0x65, 0x6d, 0x70, 0x65, 0x72,
    0x61, 0x74, 0x65, 0x00, 0x1a, 0x07, 0x0c, 0x66, 0x72, 0x6f, 0x67, 0x5f,
    0x76, 0x61, 0x72, 0x69, 0x61, 0x6e, 0x74, 0x01, 0x09, 0x74, 0x65, 0x6d,
    0x70, 0x65, 0x72, 0x61, 0x74, 0x65, 0x00, 0x18, 0x07, 0x10, 0x70, 0x61,
    0x69, 0x6e, 0x74, 0x69, 0x6e, 0x67, 0x5f, 0x76, 0x61, 0x72, 0x69, 0x61,
    0x6e, 0x74, 0x01, 0x03, 0x6f, 0x72, 0x62, 0x00, 0x19, 0x07, 0x0b, 0x70,
    0x69, 0x67, 0x5f, 0x76, 0x61, 0x72, 0x69, 0x61, 0x6e, 0x74, 0x01, 0x09,
    0x74, 0x65, 0x6d, 0x70, 0x65, 0x72, 0x61, 0x74, 0x65, 0x00, 0x1a, 0x07,
    0x12, 0x77, 0x6f, 0x6c, 0x66, 0x5f, 0x73, 0x6f, 0x75, 0x6e, 0x64, 0x5f,
    0x76, 0x61, 0x72, 0x69, 0x61, 0x6e, 0x74, 0x01, 0x03, 0x62, 0x69, 0x67,
    0x00, 0x15, 0x07, 0x0c, 0x77, 0x6f, 0x6c, 0x66, 0x5f, 0x76, 0x61, 0x72,
    0x69, 0x61, 0x6e, 0x74, 0x01, 0x04, 0x70, 0x61, 0x6c, 0x65, 0x00, 0xd7,
    0x04, 0x07, 0x0b, 0x64, 0x61, 0x6d, 0x61, 0x67, 0x65, 0x5f, 0x74, 0x79,
    0x70, 0x65, 0x31, 0x05, 0x61, 0x72, 0x72, 0x6f, 0x77, 0x00, 0x11, 0x62,
    0x61, 0x64, 0x5f, 0x72, 0x65, 0x73, 0x70, 0x61, 0x77, 0x6e, 0x5f, 0x70,
    0x6f, 0x69, 0x6e, 0x74, 0x00, 0x06, 0x63, 0x61, 0x63, 0x74, 0x75, 0x73,
    0x00, 0x08, 0x63, 0x61, 0x6d, 0x70, 0x66, 0x69, 0x72, 0x65, 0x00, 0x08,
    0x63, 0x72, 0x61, 0x6d, 0x6d, 0x69, 0x6e, 0x67, 0x00, 0x0d, 0x64, 0x72,
    0x61, 0x67, 0x6f, 0x6e, 0x5f, 0x62, 0x72, 0x65, 0x61, 0x74, 0x68, 0x00,
    0x05, 0x64, 0x72, 0x6f, 0x77, 0x6e, 0x00, 0x07, 0x64, 0x72, 0x79, 0x5f,
    0x6f, 0x75, 0x74, 0x00, 0x0b, 0x65, 0x6e, 0x64, 0x65, 0x72, 0x5f, 0x70,
    0x65, 0x61, 0x72, 0x6c, 0x00, 0x09, 0x65, 0x78, 0x70, 0x6c, 0x6f, 0x73,
    0x69, 0x6f, 0x6e, 0x00, 0x04, 0x66, 0x61, 0x6c, 0x6c, 0x00, 0x0d, 0x66,
    0x61, 0x6c, 0x6c, 0x69, 0x6e, 0x67, 0x5f, 0x61, 0x6e, 0x76, 0x69, 0x6c,
    0x00, 0x0d, 0x66, 0x61, 0x6c, 0x6c, 0x69, 0x6e, 0x67, 0x5f, 0x62, 0x6c,
    0x6f, 0x63, 0x6b, 0x00, 0x12, 0x66, 0x61, 0x6c, 0x6c, 0x69, 0x6e, 0x67,
    0x5f, 0x73, 0x74, 0x61, 0x6c, 0x61, 0x63, 0x74, 0x69, 0x74, 0x65, 0x00,
    0x08, 0x66, 0x69, 0x72, 0x65, 0x62, 0x61, 0x6c, 0x6c, 0x00, 0x09, 0x66,
    0x69, 0x72, 0x65, 0x77, 0x6f, 0x72, 0x6b, 0x73, 0x00, 0x0d, 0x66, 0x6c,
    0x79, 0x5f, 0x69, 0x6e, 0x74, 0x6f, 0x5f, 0x77, 0x61, 0x6c, 0x6c, 0x00,
    0x06, 0x66, 0x72, 0x65, 0x65, 0x7a, 0x65, 0x00, 0x07, 0x67, 0x65, 0x6e,
    0x65, 0x72, 0x69, 0x63, 0x00, 0x0c, 0x67, 0x65, 0x6e, 0x65, 0x72, 0x69,
    0x63, 0x5f, 0x6b, 0x69, 0x6c, 0x6c, 0x00, 0x09, 0x68, 0x6f, 0x74, 0x5f,
    0x66, 0x6c, 0x6f, 0x6f, 0x72, 0x00, 0x07, 0x69, 0x6e, 0x5f, 0x66, 0x69,
    0x72, 0x65, 0x00, 0x07, 0x69, 0x6e, 0x5f, 0x77, 0x61, 0x6c, 0x6c, 0x00,
    0x0e, 0x69, 0x6e, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x5f, 0x6d, 0x61,
    0x67, 0x69, 0x63, 0x00, 0x04, 0x6c, 0x61, 0x76, 0x61, 0x00, 0x0e, 0x6c,
    0x69, 0x67, 0x68, 0x74, 0x6e, 0x69, 0x6e, 0x67, 0x5f, 0x62, 0x6f, 0x6c,
    0x74, 0x00, 0x0a, 0x6d, 0x61, 0x63, 0x65, 0x5f, 0x73, 0x6d, 0x61, 0x73,
    0x68, 0x00, 0x05, 0x6d, 0x61, 0x67, 0x69, 0x63, 0x00, 0x0a, 0x6d, 0x6f,
    0x62, 0x5f, 0x61, 0x74, 0x74, 0x61, 0x63, 0x6b, 0x00, 0x13, 0x6d, 0x6f,
    0x62, 0x5f, 0x61, 0x74, 0x74, 0x61, 0x63, 0x6b, 0x5f, 0x6e, 0x6f, 0x5f,
    0x61, 0x67, 0x67, 0x72, 0x6f, 0x00, 0x0e, 0x6d, 0x6f, 0x62, 0x5f, 0x70,
    0x72, 0x6f, 0x6a, 0x65, 0x63, 0x74, 0x69, 0x6c, 0x65, 0x00, 0x07, 0x6f,
    0x6e, 0x5f, 0x66, 0x69, 0x72, 0x65, 0x00, 0x0c, 0x6f, 0x75, 0x74, 0x5f,
    0x6f, 0x66, 0x5f, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x00, 0x0e, 0x6f, 0x75,
    0x74, 0x73, 0x69, 0x64, 0x65, 0x5f, 0x62, 0x6f, 0x72, 0x64, 0x65, 0x72,
    0x00, 0x0d, 0x70, 0x6c, 0x61, 0x79, 0x65, 0x72, 0x5f, 0x61, 0x74, 0x74,
    0x61, 0x63, 0x6b, 0x00, 0x10, 0x70, 0x6c, 0x61, 0x79, 0x65, 0x72, 0x5f,
    0x65, 0x78, 0x70, 0x6c, 0x6f, 0x73, 0x69, 0x6f, 0x6e, 0x00, 0x0a, 0x73,
    0x6f, 0x6e, 0x69, 0x63, 0x5f, 0x62, 0x6f, 0x6f, 0x6d, 0x00, 0x04, 0x73,
    0x70, 0x69, 0x74, 0x00, 0x0a, 0x73, 0x74, 0x61, 0x6c, 0x61, 0x67, 0x6d,
    0x69, 0x74, 0x65, 0x00, 0x06, 0x73, 0x74, 0x61, 0x72, 0x76, 0x65, 0x00,
    0x05, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x00, 0x10, 0x73, 0x77, 0x65, 0x65,
    0x74, 0x5f, 0x62, 0x65, 0x72, 0x72, 0x79, 0x5f, 0x62, 0x75, 0x73, 0x68,
    0x00, 0x06, 0x74, 0x68, 0x6f, 0x72, 0x6e, 0x73, 0x00, 0x06, 0x74, 0x68,
    0x72, 0x6f, 0x77, 0x6e, 0x00, 0x07, 0x74, 0x72, 0x69, 0x64, 0x65, 0x6e,
    0x74, 0x00, 0x15, 0x75, 0x6e, 0x61, 0x74, 0x74, 0x72, 0x69, 0x62, 0x75,
    0x74, 0x65, 0x64, 0x5f, 0x66, 0x69, 0x72, 0x65, 0x62, 0x61, 0x6c, 0x6c,
    0x00, 0x0b, 0x77, 0x69, 0x6e, 0x64, 0x5f, 0x63, 0x68, 0x61, 0x72, 0x67,
    0x65, 0x00, 0x06, 0x77, 0x69, 0x74, 0x68, 0x65, 0x72, 0x00, 0x0c, 0x77,
    0x69, 0x74, 0x68, 0x65, 0x72, 0x5f, 0x73, 0x6b, 0x75, 0x6c, 0x6c, 0x00,
    0x46, 0x07, 0x0e, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x67, 0x65, 0x6e, 0x2f,
    0x62, 0x69, 0x6f, 0x6d, 0x65, 0x05, 0x06, 0x70, 0x6c, 0x61, 0x69, 0x6e,
    0x73, 0x00, 0x0e, 0x6d, 0x61, 0x6e, 0x67, 0x72, 0x6f, 0x76, 0x65, 0x5f,
    0x73, 0x77, 0x61, 0x6d, 0x70, 0x00, 0x06, 0x64, 0x65, 0x73, 0x65, 0x72,
    0x74, 0x00, 0x0c, 0x73, 0x6e, 0x6f, 0x77, 0x79, 0x5f, 0x70, 0x6c, 0x61,
    0x69, 0x6e, 0x73, 0x00, 0x05, 0x62, 0x65, 0x61, 0x63, 0x68, 0x00, 0x1c,
    0x07, 0x0e, 0x64, 0x69, 0x6d, 0x65, 0x6e, 0x73, 0x69, 0x6f, 0x6e, 0x5f,
    0x74, 0x79, 0x70, 0x65, 0x01, 0x09, 0x6f, 0x76, 0x65, 0x72, 0x77, 0x6f,
    0x72, 0x6c, 0x64, 0x00};

static const uint8_t tags_bin[] = {
    0x9e, 0x01, 0x0d, 0x03, 0x05, 0x66, 0x6c, 0x75, 0x69, 0x64, 0x02, 0x05,
    0x77, 0x61, 0x74, 0x65, 0x72, 0x02, 0x01, 0x02, 0x04, 0x6c, 0x61, 0x76,
    0x61, 0x02, 0x03, 0x04, 0x05, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x04, 0x10,
    0x6d, 0x69, 0x6e, 0x65, 0x61, 0x62, 0x6c, 0x65, 0x2f, 0x70, 0x69, 0x63,
    0x6b, 0x61, 0x78, 0x65, 0x14, 0x01, 0xc2, 0x04, 0x0c, 0xc7, 0x04, 0x6a,
    0xc4, 0x04, 0x88, 0x02, 0xbe, 0x01, 0x2a, 0x82, 0x02, 0x2c, 0x2e, 0xcb,
    0x07, 0xc4, 0x01, 0xae, 0x01, 0xad, 0x01, 0xc0, 0x01, 0xbb, 0x03, 0x8b,
    0x04, 0xc7, 0x07, 0x0c, 0x6d, 0x69, 0x6e, 0x65, 0x61, 0x62, 0x6c, 0x65,
    0x2f, 0x61, 0x78, 0x65, 0x06, 0x31, 0x0d, 0x47, 0xb7, 0x04, 0xc1, 0x01,
    0xbc, 0x01, 0x0f, 0x6d, 0x69, 0x6e, 0x65, 0x61, 0x62, 0x6c, 0x65, 0x2f,
    0x73, 0x68, 0x6f, 0x76, 0x65, 0x6c, 0x06, 0x08, 0x09, 0x25, 0x87, 0x02,
    0x89, 0x02, 0xa3, 0x08, 0x06, 0x6c, 0x65, 0x61, 0x76, 0x65, 0x73, 0x01,
    0x58, 0x04, 0x69, 0x74, 0x65, 0x6d, 0x01, 0x06, 0x70, 0x6c, 0x61, 0x6e,
    0x6b, 0x73, 0x01, 0x24};

/* === Main === */

int main() {
  uint32_t my_ip = IP(10, 0, 0, 2);
  uint16_t proto;
  uint8_t buf[1500], pkt[1500];
  int len, plen;
  int ipcp_done = 0, ipcp_id = 1;

/* Connection state: track a few recent connections */
#define MAX_CONNS 8
  static struct {
    uint16_t port;
    uint32_t seq;
    uint8_t mc_state;
  } conns[MAX_CONNS];
  static int num_conns = 0;

  /* Minecraft reassembly buffer - single connection */
  static uint8_t mc_buf[512];
  static int mc_buflen = 0;
  static char mc_name[32] = "Player";
  static uint8_t mc_uuid[16] = {0};

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
    else if (proto == 0x0021 && len > 40) {
      int ihl = (buf[0] & 0xF) * 4;
      uint32_t src_ip;
      memcpy(&src_ip, buf + 12, 4);
      if (buf[9] != 6)
        continue; /* TCP only */

      uint8_t *tcp = buf + ihl;
      uint16_t sp = (tcp[0] << 8) | tcp[1];
      uint16_t dp = (tcp[2] << 8) | tcp[3];
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
        if (ci < 0 && num_conns < MAX_CONNS) {
          ci = num_conns++;
          conns[ci].port = sp;
        }
        if (ci >= 0) {
          conns[ci].seq = 1001;
          conns[ci].mc_state = STATE_HANDSHAKE;
        }
        mc_buflen = 0;
        tcp_packet(pkt, &plen, dp, sp, 1000, tseq + 1, 0x12, NULL, 0, my_ip,
                   src_ip);
        ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
        ppp_send(0x0021, buf, len);
      } else if (dlen > 0 && dp == MC_PORT && ci >= 0) { /* Minecraft data */
        uint8_t *data = tcp + doff;
        uint32_t seq = conns[ci].seq;
        uint8_t mc_state = conns[ci].mc_state;

        /* ACK the data first */
        tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x10, NULL, 0, my_ip,
                   src_ip);
        ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
        ppp_send(0x0021, buf, len);

        /* Append to reassembly buffer */
        if (mc_buflen + dlen <= (int)sizeof(mc_buf)) {
          memcpy(mc_buf + mc_buflen, data, dlen);
          mc_buflen += dlen;
        }

        /* Try to parse complete packets */
        while (mc_buflen > 0) {
          int32_t pkt_len, pkt_id;
          int hdr = varint_decode(mc_buf, mc_buflen, &pkt_len);
          if (hdr < 0 || hdr + pkt_len > mc_buflen)
            break;

          uint8_t *pdata = mc_buf + hdr;
          int pid_len = varint_decode(pdata, pkt_len, &pkt_id);
          uint8_t *payload = pdata + pid_len;
          int payload_len = pkt_len - pid_len;

          uint8_t resp[1024];
          int rlen = 0;

          if (mc_state == STATE_HANDSHAKE && pkt_id == 0x00) {
            /* Handshake: protocol, addr, port, next_state */
            int32_t next_state;
            int off = varint_decode(payload, payload_len, &next_state);
            char addr[64];
            off += mc_read_string(payload + off, payload_len - off, addr, 64);
            off += 2; /* skip port */
            varint_decode(payload + off, payload_len - off, &next_state);
            mc_state = next_state;
          } else if (mc_state == STATE_LOGIN && pkt_id == 0x00) {
            /* Login Start: name, uuid */
            int off = mc_read_string(payload, payload_len, mc_name, 32);
            if (off > 0 && payload_len >= off + 16)
              memcpy(mc_uuid, payload + off, 16);

            /* Send Login Success (0x02): uuid + name + 0 properties */
            uint8_t login_data[64];
            int ld = 0;
            memcpy(login_data + ld, mc_uuid, 16);
            ld += 16;
            ld += varint_encode(strlen(mc_name), login_data + ld);
            memcpy(login_data + ld, mc_name, strlen(mc_name));
            ld += strlen(mc_name);
            ld += varint_encode(0, login_data + ld); /* 0 properties */
            rlen = mc_packet(resp, 0x02, login_data, ld);

            tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x18, resp, rlen,
                       my_ip, src_ip);
            ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
            ppp_send(0x0021, buf, len);
            seq += rlen;
          } else if (mc_state == STATE_LOGIN && pkt_id == 0x03) {
            /* Login Acknowledged -> CONFIG */
            mc_state = STATE_CONFIG;
          } else if (mc_state == STATE_CONFIG && pkt_id == 0x00) {
            /* Client Information -> send Known Packs */
            rlen = sizeof(known_packs);
            memcpy(resp, known_packs, sizeof(known_packs));
            tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x18, resp, rlen,
                       my_ip, src_ip);
            ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
            ppp_send(0x0021, buf, len);
            seq += rlen;

            /* Send Registries */
            rlen = sizeof(registries_bin);
            memcpy(resp, registries_bin, sizeof(registries_bin));
            tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x18, resp, rlen,
                       my_ip, src_ip);
            ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
            ppp_send(0x0021, buf, len);
            seq += rlen;

            /* Send Tags */
            rlen = sizeof(tags_bin);
            memcpy(resp, tags_bin, sizeof(tags_bin));
            tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x18, resp, rlen,
                       my_ip, src_ip);
            ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
            ppp_send(0x0021, buf, len);
            seq += rlen;
          } else if (mc_state == STATE_CONFIG && pkt_id == 0x07) {
            /* Client's Known Packs response -> send Finish Config */
            rlen = mc_packet(resp, 0x03, NULL, 0);
            tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x18, resp, rlen,
                       my_ip, src_ip);
            ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
            ppp_send(0x0021, buf, len);
            seq += rlen;
          } else if (mc_state == STATE_CONFIG && pkt_id == 0x03) {
            /* Acknowledge Finish Configuration -> Play state */
            mc_state = STATE_PLAY;

            /* Send Login (play) 0x2B */
            uint8_t join[128];
            int jlen = 0;
            /* Entity ID (Int, big-endian) */
            join[jlen++] = 0;
            join[jlen++] = 0;
            join[jlen++] = 0;
            join[jlen++] = 1;
            /* Is hardcore */
            join[jlen++] = 0;
            /* Dimension count = 1 */
            jlen += varint_encode(1, join + jlen);
            /* Dimension name "overworld" */
            jlen += varint_encode(9, join + jlen);
            memcpy(join + jlen, "overworld", 9);
            jlen += 9;
            /* Max players */
            jlen += varint_encode(20, join + jlen);
            /* View distance */
            jlen += varint_encode(8, join + jlen);
            /* Simulation distance */
            jlen += varint_encode(8, join + jlen);
            /* Reduced debug */
            join[jlen++] = 0;
            /* Enable respawn screen */
            join[jlen++] = 1;
            /* Do limited crafting */
            join[jlen++] = 0;
            /* Dimension type = 0 */
            jlen += varint_encode(0, join + jlen);
            /* Dimension name "overworld" */
            jlen += varint_encode(9, join + jlen);
            memcpy(join + jlen, "overworld", 9);
            jlen += 9;
            /* Hashed seed (8 bytes) */
            join[jlen++] = 0x01;
            join[jlen++] = 0x23;
            join[jlen++] = 0x45;
            join[jlen++] = 0x67;
            join[jlen++] = 0x89;
            join[jlen++] = 0xAB;
            join[jlen++] = 0xCD;
            join[jlen++] = 0xEF;
            /* Game mode (creative=1) */
            join[jlen++] = 1;
            /* Previous game mode (-1) */
            join[jlen++] = 0xFF;
            /* Is debug */
            join[jlen++] = 0;
            /* Is flat */
            join[jlen++] = 0;
            /* Has death location */
            join[jlen++] = 0;
            /* Portal cooldown */
            jlen += varint_encode(0, join + jlen);
            /* Sea level */
            jlen += varint_encode(63, join + jlen);
            /* Enforces secure chat */
            join[jlen++] = 0;

            rlen = mc_packet(resp, 0x2B, join, jlen);

            /* Synchronize Player Position (0x41) */
            uint8_t pos[128];
            int ppos = 0;
            ppos += varint_encode(-1, pos + ppos); /* Teleport ID */
            /* Position X, Y, Z (doubles) */
            uint64_t px = 0x4020000000000000ULL; /* 8.0 */
            uint64_t py = 0x4054000000000000ULL; /* 80.0 */
            uint64_t pz = 0x4020000000000000ULL; /* 8.0 */
            for (int i = 7; i >= 0; i--)
              pos[ppos++] = (px >> (i * 8)) & 0xFF;
            for (int i = 7; i >= 0; i--)
              pos[ppos++] = (py >> (i * 8)) & 0xFF;
            for (int i = 7; i >= 0; i--)
              pos[ppos++] = (pz >> (i * 8)) & 0xFF;
            /* Velocity X, Y, Z (doubles = 0) */
            memset(pos + ppos, 0, 24);
            ppos += 24;
            /* Yaw, Pitch (floats = 0) */
            memset(pos + ppos, 0, 8);
            ppos += 8;
            /* Flags (int = 0) */
            memset(pos + ppos, 0, 4);
            ppos += 4;
            rlen += mc_packet(resp + rlen, 0x41, pos, ppos);

            /* Game Event 13 - Start waiting for chunks (0x22) */
            uint8_t evt[8];
            evt[0] = 13;           /* Event ID */
            memset(evt + 1, 0, 4); /* Value (float 0) */
            rlen += mc_packet(resp + rlen, 0x22, evt, 5);

            /* Set Center Chunk (0x57) at 0,0 */
            uint8_t center[4];
            int clen = 0;
            clen += varint_encode(0, center + clen); /* X */
            clen += varint_encode(0, center + clen); /* Z */
            rlen += mc_packet(resp + rlen, 0x57, center, clen);

            tcp_packet(pkt, &plen, dp, sp, seq, tseq + dlen, 0x18, resp, rlen,
                       my_ip, src_ip);
            ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
            ppp_send(0x0021, buf, len);
            seq += rlen;
          }
          /* else ignore other config packets */

          /* Remove processed packet from buffer */
          int total = hdr + pkt_len;
          memmove(mc_buf, mc_buf + total, mc_buflen - total);
          mc_buflen -= total;
        }

        conns[ci].seq = seq;
        conns[ci].mc_state = mc_state;
      } else if (flags & 0x01 && ci >= 0) { /* FIN -> FIN-ACK */
        uint32_t seq = conns[ci].seq;
        tcp_packet(pkt, &plen, dp, sp, seq, tseq + 1, 0x11, NULL, 0, my_ip,
                   src_ip);
        ip_packet(buf, &len, my_ip, src_ip, pkt, plen);
        ppp_send(0x0021, buf, len);
        /* Remove connection */
        conns[ci] = conns[--num_conns];
      }
    }
  }
  return 0;
}
