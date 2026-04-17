/*
 * AES-256-CBC Encrypt/Decrypt for UNIVAC
 * Uses tiny-AES-c library
 *
 * Key:  64 hex chars (256 bits)
 * IV:   32 hex chars (128 bits)
 * Data: Hex encoded (for decrypt) or raw text (for encrypt)
 */

#include <stdio.h>
#include <string.h>
#include "aes.h"

#define MAX_INPUT 4096
#define BLOCK_SIZE 16

/* Convert hex char to nibble, -1 on error */
static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* Convert hex string to bytes, returns bytes written or -1 on error */
static int hex_to_bytes(const char *hex, uint8_t *out, int max_bytes) {
    int len = strlen(hex);
    int bytes = 0;
    for (int i = 0; i + 1 < len && bytes < max_bytes; i += 2) {
        int hi = hex_nibble(hex[i]);
        int lo = hex_nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return -1;
        out[bytes++] = (hi << 4) | lo;
    }
    return bytes;
}

/* Convert bytes to uppercase hex */
static void bytes_to_hex(const uint8_t *in, int len, char *out) {
    const char *digits = "0123456789ABCDEF";
    for (int i = 0; i < len; i++) {
        out[i * 2] = digits[in[i] >> 4];
        out[i * 2 + 1] = digits[in[i] & 0x0F];
    }
    out[len * 2] = '\0';
}

/* Read line, strip newline, return length */
static int read_line(char *buf, int max) {
    if (fgets(buf, max, stdin) == NULL) return -1;
    int len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
        buf[--len] = '\0';
    return len;
}

/* Add PKCS7 padding */
static int pkcs7_pad(uint8_t *data, int len) {
    int pad_len = BLOCK_SIZE - (len % BLOCK_SIZE);
    for (int i = 0; i < pad_len; i++)
        data[len + i] = pad_len;
    return len + pad_len;
}

/* Remove PKCS7 padding, returns unpadded length or -1 on error */
static int pkcs7_unpad(uint8_t *data, int len) {
    if (len == 0 || len % BLOCK_SIZE != 0) return -1;
    uint8_t pad = data[len - 1];
    if (pad == 0 || pad > BLOCK_SIZE) return -1;
    for (int i = 0; i < pad; i++) {
        if (data[len - 1 - i] != pad) return -1;
    }
    return len - pad;
}

int main(void) {
    char line[MAX_INPUT];
    uint8_t key[32], iv[16];
    uint8_t data[MAX_INPUT];
    struct AES_ctx ctx;
    int mode; /* 0=encrypt, 1=decrypt */

    printf("AES-256-CBC\n");
    printf("===========\n\n");

    /* Mode selection */
    printf("MODE (E=ENCRYPT, D=DECRYPT): ");
    fflush(stdout);
    read_line(line, sizeof(line));
    if (line[0] == 'D' || line[0] == 'd') {
        mode = 1;
        printf("DECRYPT MODE\n\n");
    } else {
        mode = 0;
        printf("ENCRYPT MODE\n\n");
    }

    /* Default key and IV */
    static const char *default_key_hex =
        "0123456789ABCDEF0123456789ABCDEF"
        "0123456789ABCDEF0123456789ABCDEF";
    static const char *default_iv_hex =
        "FEDCBA9876543210FEDCBA9876543210";

    /* Get key */
    printf("KEY (64 HEX CHARS, ENTER FOR DEFAULT): ");
    fflush(stdout);
    int key_len = read_line(line, sizeof(line));
    if (key_len == 0) {
        strcpy(line, default_key_hex);
        printf("USING DEFAULT KEY:\n%s\n", line);
    }
    if (strlen(line) < 64) {
        printf("ERROR: NEED 64 HEX CHARS\n");
        return 1;
    }
    if (hex_to_bytes(line, key, 32) != 32) {
        printf("ERROR: INVALID HEX IN KEY\n");
        return 1;
    }

    /* Get IV */
    printf("IV (32 HEX CHARS, ENTER FOR DEFAULT): ");
    fflush(stdout);
    int iv_len = read_line(line, sizeof(line));
    if (iv_len == 0) {
        strcpy(line, default_iv_hex);
        printf("USING DEFAULT IV:  %s\n", line);
    }
    if (strlen(line) < 32) {
        printf("ERROR: NEED 32 HEX CHARS\n");
        return 1;
    }
    if (hex_to_bytes(line, iv, 16) != 16) {
        printf("ERROR: INVALID HEX IN IV\n");
        return 1;
    }

    /* Initialize AES context */
    AES_init_ctx_iv(&ctx, key, iv);

    if (mode == 0) {
        /* ENCRYPT: Read plaintext, output hex ciphertext */
        printf("\nPLAINTEXT (END WITH #):\n");
        fflush(stdout);

        int len = 0;
        int c;
        while ((c = getchar()) != EOF && c != '#' && len < MAX_INPUT - BLOCK_SIZE) {
            data[len++] = (uint8_t)c;
        }

        /* Pad and encrypt */
        len = pkcs7_pad(data, len);
        AES_CBC_encrypt_buffer(&ctx, data, len);

        /* Output as hex */
        printf("\nCIPHERTEXT:\n");
        bytes_to_hex(data, len, line);
        printf("%s\n", line);

    } else {
        /* DECRYPT: Read hex ciphertext, output plaintext */
        printf("\nCIPHERTEXT (HEX): ");
        fflush(stdout);
        int hex_len = read_line(line, sizeof(line));

        int len = hex_to_bytes(line, data, MAX_INPUT);
        if (len < 0 || len % BLOCK_SIZE != 0) {
            printf("ERROR: INVALID CIPHERTEXT\n");
            return 1;
        }

        /* Decrypt and unpad */
        AES_CBC_decrypt_buffer(&ctx, data, len);
        len = pkcs7_unpad(data, len);
        if (len < 0) {
            printf("ERROR: INVALID PADDING\n");
            return 1;
        }

        /* Output plaintext */
        data[len] = '\0';
        printf("\nPLAINTEXT:\n%s\n", (char *)data);
    }

    fflush(stdout);
    return 0;
}
