/*
 * Program Name: CHAT
 * Program Release Year: 2025
 * Program Author: Z80-uLM Project (Auto-generated)
 * 
 * Neural Network Chat Application
 * Autoregressive character-by-character text generation
 * 
 * Architecture: 256 -> 256 -> 192 -> 128 -> 40
 */

#ifndef CHAT_H
#define CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Platform-specific includes
#ifndef UNIVAC
#ifdef _WIN32
#include <windows.h>
#endif
#endif

// Constants
#define INPUT_SIZE 256           // 128 query + 128 context buckets
#define NUM_QUERY_BUCKETS 128
#define NUM_CONTEXT_BUCKETS 128
#define NUM_CHARS 40             // Characters in charset (including EOS)
#define EOS_IDX (39)           // End-of-sequence index
#define MAX_OUTPUT_LEN 50   // Maximum output characters
#define MAX_INPUT_LEN 256                 // Input buffer size
#define CONTEXT_LEN 8                     // Number of context characters
#define NUM_LAYERS 4

// Layer sizes
#define LAYER_INPUT_SIZE 256
#define LAYER1_SIZE 256
#define LAYER2_SIZE 192
#define LAYER3_SIZE 128
#define LAYER4_SIZE 40  // Output
#define MAX_HIDDEN_SIZE 256

// Fixed-point scaling
#define ACTIVATION_SCALE 32

// State structure
typedef struct {
    short input_buf[INPUT_SIZE];     // Combined query + context buckets
    short hidden_a[MAX_HIDDEN_SIZE]; // Hidden buffer A
    short hidden_b[MAX_HIDDEN_SIZE]; // Hidden buffer B
    short output_buf[NUM_CHARS];     // Output buffer
    char context_chars[CONTEXT_LEN]; // Last N output characters
    char user_input[MAX_INPUT_LEN];  // User input buffer
} ChatState;

// Function declarations

// Initialization
void init_chat(ChatState* state);
void clear_context(ChatState* state);

// Main loop
void run_chat(ChatState* state);
void process_query(ChatState* state, const char* query);
void generate_response(ChatState* state);

// Neural network inference
void run_inference(ChatState* state);
void layer_forward(const unsigned char* weights, const short* biases,
                   const short* input, short* output,
                   int in_size, int out_size, int apply_relu);
short muladd(short acc, signed char weight, short activation);

// Encoding
void tokenize_query(ChatState* state, const char* query);
void encode_context(ChatState* state);
void update_context(ChatState* state, char new_char);

// Utility
int find_argmax(const short* buf, int size);
void to_lowercase(char* str);
int hash_trigram(const char* trigram);
int hash_ngram(const char* ngram, int len, int offset);

// Platform-specific
#ifndef UNIVAC
void console_setup(void);
#endif

// Weights and biases (defined in CHAT.c)
extern const unsigned char layer1_weights[16384];
extern const short layer1_biases[256];
extern const unsigned char layer2_weights[12288];
extern const short layer2_biases[192];
extern const unsigned char layer3_weights[6144];
extern const short layer3_biases[128];
extern const unsigned char layer4_weights[1280];
extern const short layer4_biases[40];

// Character table
extern const char charset[NUM_CHARS];

#endif // CHAT_H
