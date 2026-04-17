/*
 * ELIZA/DOCTOR Simulation
 * Modern C implementation for Windows Console
 * Created by Joseph Weizenbaum
 * This version by Jeff Shrager
 * Edited and modified for MITS 8K BASIC 4.0 by Steve North
 * From Creative Computing Jul-Aug 1977
 * 
 * Rewritten in C for Windows conhost terminal
 * Compiles with MSVC and MinGW
 */

#ifndef ELIZA_H
#define ELIZA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Platform-specific includes
#ifndef UNIVAC
#include <windows.h>
#endif

// Constants
#define MAX_STRING_LEN 73      // 72 chars + null terminator
#define MAX_INPUT_LEN 256      // Generous buffer for user input
#define NUM_KEYWORDS 36        // N1 in original
#define NUM_CONJUGATIONS 12    // N2 in original (6 pairs)
#define NUM_REPLIES 112        // N3 in original

// Keyword indices (for reference)
#define KW_CAN_YOU 0
#define KW_CAN_I 1
#define KW_YOU_ARE 2
#define KW_YOURE 3
#define KW_I_DONT 4
#define KW_I_FEEL 5
#define KW_WHY_DONT_YOU 6
#define KW_WHY_CANT_I 7
#define KW_ARE_YOU 8
#define KW_I_CANT 9
#define KW_I_AM 10
#define KW_IM 11
#define KW_YOU 12
#define KW_I_WANT 13
#define KW_WHAT 14
#define KW_HOW 15
#define KW_WHO 16
#define KW_WHERE 17
#define KW_WHEN 18
#define KW_WHY 19
#define KW_NAME 20
#define KW_CAUSE 21
#define KW_SORRY 22
#define KW_DREAM 23
#define KW_HELLO 24
#define KW_HI 25
#define KW_MAYBE 26
#define KW_NO 27
#define KW_YOUR 28
#define KW_ALWAYS 29
#define KW_THINK 30
#define KW_ALIKE 31
#define KW_YES 32
#define KW_FRIEND 33
#define KW_COMPUTER 34
#define KW_NOKEYFOUND 35

// Conjugation pair structure
typedef struct {
    const char* from;
    const char* to;
} ConjugationPair;

// Reply range structure
typedef struct {
    int start;      // S(X) - starting index in replies array
    int count;      // L - number of replies for this keyword
    int current;    // R(X) - current reply index (for cycling)
    int end;        // N(X) - ending index in replies array
} ReplyRange;

// Game state structure
typedef struct {
    char input[MAX_STRING_LEN];       // I$ - Current input
    char previous[MAX_STRING_LEN];    // P$ - Previous input
    char conjugated[MAX_STRING_LEN];  // C$ - Conjugated response string
    char keyword[MAX_STRING_LEN];     // F$ - Found keyword
    
    ReplyRange reply_ranges[NUM_KEYWORDS];  // S(), R(), N() arrays
    
    int keyword_index;  // K - Current keyword index
    int keyword_pos;    // L - Position where keyword was found
} ElizaState;

// Keyword data (in priority order)
extern const char* keywords[NUM_KEYWORDS];

// Conjugation pairs for transforming user input
extern const ConjugationPair conjugations[NUM_CONJUGATIONS / 2];

// Reply templates
extern const char* replies[NUM_REPLIES];

// Reply range initialization data (matches DATA statement at line 2530-2560)
extern const int reply_init_data[NUM_KEYWORDS * 2];

// Function declarations

// Initialization
void init_eliza(ElizaState* state);
void init_reply_ranges(ElizaState* state);

// Main conversation loop
void run_eliza(ElizaState* state);

// Input processing
void get_user_input(ElizaState* state);
void remove_apostrophes(char* str);
void check_for_quit(const char* str);

// Keyword matching
int find_keyword(ElizaState* state);

// String conjugation
void conjugate_string(ElizaState* state);
void swap_words(char* str, const char* from, const char* to);

// Reply generation
void generate_reply(ElizaState* state);
const char* get_next_reply(ElizaState* state, int keyword_idx);

// Utility functions
void to_uppercase(char* str);
void trim_string(char* str);
int str_find(const char* haystack, const char* needle);
void str_replace_at(char* dest, size_t dest_size, int pos, int len, const char* replacement);

// Platform-specific functions
#ifndef UNIVAC
void console_setup(void);
#endif

#endif // ELIZA_H
