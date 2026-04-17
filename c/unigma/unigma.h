/*
 * Unigma: The Little UNIVAC Enigma Simulator
 * ROTORS: I, II, III | REFLECTOR: B
 *
 * Modern C implementation for Windows Console and UNIVAC
 * Portable implementation with platform-specific support
 */

#ifndef UNIGMA_H
#define UNIGMA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Platform-specific includes
#ifndef UNIVAC
#include <windows.h>
#endif

// Constants
#define NUM_ROTORS 3
#define NUM_ROTOR_WIRINGS 4  // 3 rotors + 1 reflector
#define ALPHABET_SIZE 26
#define MAX_PLUGBOARD_LEN 256

// Rotor wiring structure
typedef struct {
    char wiring[ALPHABET_SIZE + 1];  // Rotor wiring configuration (null-terminated string)
} RotorWiring;

// Enigma machine state
typedef struct {
    // Rotor wirings: I, II, III, and Reflector B
    RotorWiring rotors[NUM_ROTOR_WIRINGS];

    // Notch positions for stepping: Q, E, V (for I, II, III)
    int notch_positions[NUM_ROTORS];

    // Current rotor positions (0-25, corresponding to A-Z)
    // p[0] = right rotor (rotor I), p[1] = middle rotor (rotor II), p[2] = left rotor (rotor III)
    int positions[NUM_ROTORS];

    // Plugboard configuration (e.g., "AB CD EF" swaps A<->B, C<->D, E<->F)
    char plugboard[MAX_PLUGBOARD_LEN];
} EnigmaState;

// Function declarations

// Initialization
void init_enigma(EnigmaState* state);
void init_rotors(EnigmaState* state);
void init_notches(EnigmaState* state);
void init_positions(EnigmaState* state);
void init_plugboard(EnigmaState* state);

// Runtime configuration
void parse_arguments(int argc, char* argv[], EnigmaState* state);
void interactive_config(EnigmaState* state);
void set_rotor_positions(EnigmaState* state, const char* positions);
void set_plugboard(EnigmaState* state, const char* plugboard_config);
void print_usage(const char* program_name);
void print_current_config(const EnigmaState* state);

// Helper functions
int idx(const char* s, int c);
int mod_positive(int a);

// Encryption functions
int encrypt_char(int c, EnigmaState* state);
int encode_through_rotor(int input_char, int rotor_index, int position, int direction, const EnigmaState* state);
char apply_plugboard(char c, const char* plugboard);

// Main encryption loop
void run_enigma(EnigmaState* state);

// Stepping mechanism
void step_rotors(EnigmaState* state);

// Platform-specific functions
#ifndef UNIVAC
void console_setup(void);
#endif

#endif // UNIGMA_H
