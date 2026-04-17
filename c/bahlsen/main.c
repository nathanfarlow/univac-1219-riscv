/* Bahlsen: UNIVAC Enigma Brute-Forcer */
/* ROTORS: I, II, III (Fixed Order) | REF: B */
/* CRACKS BY ROTOR START POSITION ONLY */

#include <stdio.h>
#include <string.h>

#ifndef UNIVAC
#include <windows.h>
#include <conio.h>
#else
/* UNIVAC compatibility - no Windows dependencies */
#define strncpy_s(dest, size, src, count) strncpy(dest, src, count)
#define strcpy_s(dest, size, src) strncpy(dest, src, size-1)
#endif

/* --- GLOBALS --- */
/* Standard Wehrmacht Rotors - BSS initialized */
static char R[4][27] = {
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ", /* I   */
    "AJDKSIRUXBLHWTMCQGZNPYFVOE", /* II  */
    "BDFHJLCPRTXVZNYEIWGAKMUSQO", /* III */
    "YRUHQSLDPXNGOKMIEBFZCWVJAT"  /* Ref B */
};
/* Notches: Q, E, V - BSS initialized */
static int N[3] = { 16, 4, 21 }; 

/* --- HELPERS --- */
/* Index of char in string */
int idx(char *s, int c) {
    char *p = s; 
    int i = 0;
    while (*p) { 
        if (*p == c) return i; 
        p++; 
        i++; 
    }
    return -1;
}

/* Positive Modulo */
int mod(int a) { 
    return (a % 26 + 26) % 26; 
}

/* Enigma Map Function */
int map(int k, int r, int p, int d) {
    int i = mod(k + p);
    if (d == 0) return mod(idx("ABCDEFGHIJKLMNOPQRSTUVWXYZ", R[r][i]) - p);
    else        return mod(idx(R[r], "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i]) - p);
}

/* Simple Substring Check */
int has_crib(char *txt, char *crib) {
    int i, j;
    for (i = 0; txt[i] != '\0'; i++) {
        for (j = 0; crib[j] != '\0'; j++) {
            if (txt[i+j] != crib[j]) break;
        }
        if (crib[j] == '\0') return 1; /* Found */
    }
    return 0;
}

/* Cross-platform input function */
int safe_getchar(void) {
#ifndef UNIVAC
    return _getch();
#else
    return getchar();
#endif
}

/* --- MAIN CRACKER --- */
int main(void) {
    /* Buffers - BSS initialized */
    static char cipher[80] = {0};
    static char crib[80] = {0};
    static char plain[80] = {0};
    
    /* Loop Vars - BSS initialized */
    int i = 0, j = 0, k = 0, x = 0, len = 0;
    int c = 0;
    static int t_p[3] = {0}; /* Temp Rotor Pos */
    
    /* 1. INPUT PHASE */
#ifndef UNIVAC
    /* Windows console setup */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    
    printf("Bahlsen V1.0 - UNIVAC Enigma Brute-Forcer\n");
    printf("Compatible with Windows and UNIVAC mainframes\n\n");
    printf("ENTER CIPHER: ");
    fflush(stdout);
    
    /* Safe input with bounds checking */
    i = 0;
    while ((c = getchar()) != '\n' && c != EOF && i < 79) {
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= 'A' && c <= 'Z') cipher[i++] = c;
    }
    cipher[i] = '\0';
    len = i;

    printf("ENTER CRIB:   ");
    fflush(stdout);
    
    i = 0;
    while ((c = getchar()) != '\n' && c != EOF && i < 79) {
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= 'A' && c <= 'Z') crib[i++] = c;
    }
    crib[i] = '\0';

    printf("\nCRUNCHING [.=676 CHECKS]\n");

    /* 2. BRUTE FORCE LOOPS (Left, Middle, Right) */
    /* Order: p[2]=L, p[1]=M, p[0]=R */
    for (k = 0; k < 26; k++) {      /* Left Rotor */
        printf("."); /* Heartbeat */
        for (j = 0; j < 26; j++) {  /* Middle Rotor */
            for (i = 0; i < 26; i++) { /* Right Rotor */
                
                /* Reset Temp Rotors for this Attempt */
                t_p[0] = i; t_p[1] = j; t_p[2] = k;

                /* Decrypt Cipher with Current Settings */
                for (x = 0; x < len; x++) {
                    /* Step */
                    if (t_p[1] == N[1]) {
                        t_p[1] = mod(t_p[1] + 1);
                        t_p[2] = mod(t_p[2] + 1);
                    } else if (t_p[0] == N[0]) {
                        t_p[1] = mod(t_p[1] + 1);
                    }
                    t_p[0] = mod(t_p[0] + 1);

                    /* Decrypt Path */
                    c = cipher[x] - 'A';
                    c = map(c, 2, t_p[0], 0); /* R */
                    c = map(c, 1, t_p[1], 0); /* M */
                    c = map(c, 0, t_p[2], 0); /* L */
                    c = idx("ABCDEFGHIJKLMNOPQRSTUVWXYZ", R[3][c]); /* Ref */
                    c = map(c, 0, t_p[2], 1); /* L */
                    c = map(c, 1, t_p[1], 1); /* M */
                    c = map(c, 2, t_p[0], 1); /* R */
                    
                    plain[x] = c + 'A';
                }
                plain[len] = '\0';

                /* 3. CHECK MATCH */
                if (has_crib(plain, crib)) {
                    printf("\n\n[!] MATCH FOUND [!]\n");
                    printf("SETTINGS (L M R): %c %c %c\n", k+'A', j+'A', i+'A');
                    printf("PLAINTEXT: %s\n", plain);
#ifndef UNIVAC
                    printf("\nPress any key to exit...\n");
                    _getch();
#endif
                    return 0; /* Stop on first match */
                }
            }
        }
    }
    printf("\nNO MATCH FOUND.\n");
#ifndef UNIVAC
    printf("Press any key to exit...\n");
    _getch();
#endif
    return 0;
}