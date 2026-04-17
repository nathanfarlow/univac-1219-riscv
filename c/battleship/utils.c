/*
 * utils.c - Utility functions for Battleship game
 * Cross-platform compatible with XorShift RNG for UNIVAC
 */

#include "battleship.h"

/* XorShift32 random number generator - works on all platforms */
unsigned int xorshift32(unsigned int* state) {
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Initialize random number generator with time(0) */
void init_random(unsigned int* state) {
    #ifdef UNIVAC
    /* UNIVAC: Use time(0) XOR'd with a constant for better distribution */
    *state = (unsigned int)time(NULL) ^ 0x5EED5EED;
    #else
    /* Windows: Can use time(0) or more sophisticated seeding */
    *state = (unsigned int)time(NULL);
    #endif

    /* Warm up the RNG */
    xorshift32(state);
    xorshift32(state);
    xorshift32(state);
}

/* Generate random number in range [min, max] inclusive */
int random_range(unsigned int* state, int min, int max) {
    unsigned int r = xorshift32(state);
    return min + (r % (max - min + 1));
}

/* Generate random row ('A' to 'J') */
char random_row(unsigned int* state) {
    return 'A' + (char)random_range(state, 0, 9);
}

/* Generate random column (1 to 10) */
int random_col(unsigned int* state) {
    return random_range(state, 1, 10);
}

/* Clear the screen - platform dependent */
void clear_screen(void) {
    #ifdef UNIVAC
    /* UNIVAC: Simple newlines */
    int i;
    for (i = 0; i < 25; i++) {
        printf("\n");
    }
    #elif defined(_WIN32)
    /* Windows: Use system command */
    system("CLS");
    #else
    /* Unix/Linux: Use system command */
    system("CLEAR");
    #endif
}

/* Prompt user to press Enter */
void prompt_enter_key(void) {
    printf("PRESS ENTER FOR THE NEXT STEP\n");
    getchar();
    clear_screen();
}

/* Print a divider line */
void print_divider(void) {
    int i;
    for (i = 0; i < 20; i++) {
        printf("-");
    }
    printf("\n");
}

/* Get input from user */
void get_input(char* buffer, int size) {
    if (fgets(buffer, size, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
    }
}

/* Normalize coordinates so first is always <= second */
void normalize_coordinates(char* roF, char* roS, int* coF, int* coS) {
    char temp_char;
    int temp_int;

    if (*roF > *roS) {
        temp_char = *roF;
        *roF = *roS;
        *roS = temp_char;
    }

    if (*coF > *coS) {
        temp_int = *coF;
        *coF = *coS;
        *coS = temp_int;
    }
}
