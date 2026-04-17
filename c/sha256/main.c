#define LONESHA256_STATIC
#include "lonesha256.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    char input[1024];
    unsigned char hash[32];

    printf("SHA-256 HASH UTILITY\n");
    printf("ENTER TEXT TO HASH (EMPTY LINE TO QUIT):\n\n");

    for (;;) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin))
            break;

        /* strip trailing newline */
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n')
            input[--len] = '\0';

        if (len == 0)
            break;

        lonesha256(hash, (const unsigned char *)input, len);

        for (int i = 0; i < 32; i++)
            printf("%02X", hash[i]);
        printf("\n\n");
    }

    return 0;
}
