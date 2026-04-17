#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

extern const uint32_t all_words[];
extern const int all_count;
extern const uint16_t sol_idx[];
extern const int sol_count;

static uint32_t encode(const char *w) {
    uint32_t v = 0;
    for (int i = 0; i < 5; i++)
        v = (v << 5) | (w[i] - 'A');
    return v;
}

static void decode(uint32_t v, char *out) {
    for (int i = 4; i >= 0; i--) {
        out[i] = 'A' + (v & 0x1F);
        v >>= 5;
    }
    out[5] = 0;
}

/* read a line, strip newline, return length. -1 on EOF */
static int readln(char *buf, int max) {
    int c, n = 0;
    while (n < max - 1) {
        c = getchar();
        if (c == EOF) return n ? n : -1;
        if (c == '\n' || c == '\r') break;
        buf[n++] = c;
    }
    buf[n] = 0;
    /* drain rest of line */
    if (c != '\n' && c != '\r' && c != EOF)
        while ((c = getchar()) != EOF && c != '\n' && c != '\r');
    return n;
}

static void uppercase(char *s) {
    for (; *s; s++)
        if (*s >= 'a' && *s <= 'z') *s &= ~0x20;
}

static int valid(const char *w) {
    for (int i = 0; i < 5; i++)
        if (w[i] < 'A' || w[i] > 'Z') return 0;
    return 1;
}

/* binary search */
static int has_word(uint32_t v) {
    int lo = 0, hi = all_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (all_words[mid] == v) return 1;
        if (all_words[mid] < v) lo = mid + 1;
        else hi = mid - 1;
    }
    return 0;
}

static void check(const char *guess, const char *answer) {
    char result[6] = "-----";
    char copy[6];
    memcpy(copy, answer, 5);
    copy[5] = 0;
    for (int i = 0; i < 5; i++) {
        if (guess[i] == copy[i]) {
            result[i] = '#';
            copy[i] = 0;
        }
    }
    for (int i = 0; i < 5; i++) {
        if (result[i] == '#') continue;
        for (int j = 0; j < 5; j++) {
            if (copy[j] == guess[i]) {
                result[i] = 'O';
                copy[j] = 0;
                break;
            }
        }
    }
    puts(result);
}

static int my_atoi(const char *s) {
    int n = 0;
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');
    return n;
}

int main(void) {
    char answer[6], guess[8], alpha[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char buf[21];
    int guesses = 0, game, n;

    setbuf(stdout, NULL);
    srand((int)time(0));

    printf("WORDLE (%d WORDS)\nNEW OR 1-%d: ", sol_count, sol_count);

    if (readln(buf, sizeof buf) < 0) return 0;
    uppercase(buf);

    if (strcmp(buf, "NEW") == 0)
        game = rand() % sol_count;
    else {
        game = my_atoi(buf) - 1;
        if (game < 0 || game >= sol_count)
            game = rand() % sol_count;
    }

    decode(all_words[sol_idx[game]], answer);
    printf("GAME #%d  (#=RIGHT O=MISPLACED -=NO)\n", game + 1);

    while (guesses < 6) {
        printf("%d> ", guesses + 1);
        n = readln(guess, sizeof guess);
        if (n < 0) break;
        uppercase(guess);

        if (n != 5 || !valid(guess)) {
            puts("5 LETTERS A-Z");
            continue;
        }
        if (!has_word(encode(guess))) {
            puts("NOT IN LIST");
            continue;
        }
        if (strcmp(guess, answer) == 0) {
            puts("#####");
            printf("WIN IN %d!\n", guesses + 1);
            return 0;
        }
        guesses++;
        check(guess, answer);
        for (int i = 0; i < 5; i++) {
            int pos = guess[i] - 'A';
            if (pos >= 0 && pos < 26) alpha[pos] = '.';
        }
        printf("  %s\n", alpha);
    }
    printf("ANSWER: %s\n", answer);
    return 0;
}
