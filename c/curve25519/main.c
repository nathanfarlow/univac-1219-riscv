/*
 * X25519 Key Exchange Demo for UNIVAC
 * Uses TweetNaCl's crypto_scalarmult (Curve25519)
 *
 * Flow:
 *   1. Enter your private key (or use demo key)
 *   2. Get your public key
 *   3. Enter peer's public key
 *   4. Get shared secret (use as AES-256 key!)
 */

#include <stdio.h>
#include <string.h>

/* X25519 functions from tweetnacl.c */
int crypto_scalarmult(unsigned char *q, const unsigned char *n, const unsigned char *p);
int crypto_scalarmult_base(unsigned char *q, const unsigned char *n);

/* randombytes stub - we use user-provided keys */
void randombytes(unsigned char *x, unsigned long long n) {
    (void)x; (void)n;
}

/* Convert hex char to nibble */
static int hex_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* Convert 64 hex chars to 32 bytes */
static int hex_to_bytes(const char *hex, unsigned char *out) {
    for (int i = 0; i < 32; i++) {
        int hi = hex_to_nibble(hex[i*2]);
        int lo = hex_to_nibble(hex[i*2 + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[i] = (hi << 4) | lo;
    }
    return 0;
}

/* Convert 32 bytes to 64 hex chars (uppercase for UNIVAC) */
static void bytes_to_hex(const unsigned char *in, char *hex) {
    const char *digits = "0123456789ABCDEF";
    for (int i = 0; i < 32; i++) {
        hex[i*2] = digits[in[i] >> 4];
        hex[i*2 + 1] = digits[in[i] & 0x0F];
    }
    hex[64] = '\0';
}

/* Read a line, strip newline */
static int read_line(char *buf, int max) {
    if (fgets(buf, max, stdin) == NULL) return -1;
    int len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') buf[--len] = '\0';
    if (len > 0 && buf[len-1] == '\r') buf[--len] = '\0';
    return len;
}

/* Demo private key (for testing - RFC 7748 test vector) */
static const char *demo_private =
    "77076D0A7318A57D3C16C17251B26645DF4C2F87EBC0992AB177FBA51DB92C2A";

int main(void) {
    unsigned char private_key[32];
    unsigned char public_key[32];
    unsigned char peer_public[32];
    unsigned char shared_secret[32];
    char hex[128];

    printf("X25519 KEY EXCHANGE\n");
    printf("===================\n\n");

    /* Step 1: Get private key */
    printf("PRIVATE KEY (64 HEX, OR ENTER FOR DEMO):\n");
    fflush(stdout);

    int len = read_line(hex, sizeof(hex));

    if (len < 64) {
        printf("USING DEMO KEY\n");
        strcpy(hex, demo_private);
    }

    if (hex_to_bytes(hex, private_key) < 0) {
        printf("ERROR: INVALID HEX\n");
        return 1;
    }

    /* Step 2: Compute and show public key */
    crypto_scalarmult_base(public_key, private_key);
    bytes_to_hex(public_key, hex);

    printf("\nYOUR PUBLIC KEY:\n");
    printf("%s\n", hex);
    fflush(stdout);

    /* Step 3: Get peer's public key */
    printf("\nPEER PUBLIC KEY (64 HEX):\n");
    fflush(stdout);

    len = read_line(hex, sizeof(hex));

    if (len < 64 || hex_to_bytes(hex, peer_public) < 0) {
        printf("ERROR: NEED 64 HEX CHARS\n");
        return 1;
    }

    /* Step 4: Compute shared secret */
    crypto_scalarmult(shared_secret, private_key, peer_public);
    bytes_to_hex(shared_secret, hex);

    printf("\nSHARED SECRET:\n");
    printf("%s\n", hex);
    printf("\n(USE AS AES-256 KEY WITH CHIPANCHOR)\n");
    fflush(stdout);

    return 0;
}
