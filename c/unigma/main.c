/*
 * Unigma: The Little UNIVAC Enigma Simulator
 * ROTORS: I, II, III | REFLECTOR: B
 *
 * Main implementation
 * Portable C implementation for Windows Console and UNIVAC
 */

#include "unigma.h"

// Platform-specific string copy
#ifdef UNIVAC
#define SAFE_STRCPY(dest, src, size)                                           \
  do {                                                                         \
    strncpy(dest, src, (size) - 1);                                            \
    (dest)[(size) - 1] = '\0';                                                 \
  } while (0)
#else
#define SAFE_STRCPY(dest, src, size) strcpy_s(dest, size, src)
#endif

// Rotor wiring constants (matches original R[] array)
// Input A..Z maps to...
const char *ROTOR_WIRINGS[NUM_ROTOR_WIRINGS] = {
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ", /* Rotor I   */
    "AJDKSIRUXBLHWTMCQGZNPYFVOE", /* Rotor II  */
    "BDFHJLCPRTXVZNYEIWGAKMUSQO", /* Rotor III */
    "YRUHQSLDPXNGOKMIEBFZCWVJAT"  /* Reflector B */
};

// Notch positions constants (matches original N[] array)
// Q, E, V for rotors I, II, III (16, 4, 21 in 0-indexed)
const int NOTCH_POSITIONS_INIT[NUM_ROTORS] = {16, 4, 21};

// Alphabet constant for reference
static const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Main entry point
int main(void) {
  int argc = 0;
  char **argv = NULL;
#ifndef UNIVAC
  console_setup();
#endif

  EnigmaState state;
  init_enigma(&state);

  // If no command-line arguments, use interactive configuration
  if (argc == 0) {
    interactive_config(&state);
  } else {
    // Parse command-line arguments for runtime configuration
    parse_arguments(argc, argv, &state);
  }

  run_enigma(&state);

  return 0;
}

// Initialize Enigma machine state
void init_enigma(EnigmaState *state) {
  // BSS initialization - zero out entire state structure
  memset(state, 0, sizeof(EnigmaState));

  // Initialize rotor wirings
  init_rotors(state);

  // Initialize notch positions
  init_notches(state);

  // Initialize rotor positions (start setting: A A A)
  init_positions(state);

  // Initialize plugboard (empty by default)
  init_plugboard(state);
}

// Initialize rotor wirings
void init_rotors(EnigmaState *state) {
  for (int i = 0; i < NUM_ROTOR_WIRINGS; i++) {
    SAFE_STRCPY(state->rotors[i].wiring, ROTOR_WIRINGS[i], ALPHABET_SIZE + 1);
  }
}

// Initialize notch positions
void init_notches(EnigmaState *state) {
  for (int i = 0; i < NUM_ROTORS; i++) {
    state->notch_positions[i] = NOTCH_POSITIONS_INIT[i];
  }
}

// Initialize rotor positions (A A A = 0, 0, 0)
void init_positions(EnigmaState *state) {
  for (int i = 0; i < NUM_ROTORS; i++) {
    state->positions[i] = 0;
  }
}

// Initialize plugboard (empty by default)
void init_plugboard(EnigmaState *state) {
  // Plugboard: Swaps pairs. Example: "AB CD" swaps A<->B and C<->D
  // Add pairs here as needed: "AB CD EF ..."
  state->plugboard[0] = '\0'; // Empty plugboard by default
}

// Helper: Find index of char c in string s
int idx(const char *s, int c) {
  const char *p = s;
  while (*p) {
    if (*p == c) {
      return (int)(p - s);
    }
    p++;
  }
  return -1;
}

// Helper: Modulo 26 positive
int mod_positive(int a) {
  return (a % ALPHABET_SIZE + ALPHABET_SIZE) % ALPHABET_SIZE;
}

// Helper: Map through rotor
// k: input character (0-25)
// rotor_index: which rotor to use (0=I, 1=II, 2=III, 3=Reflector)
// position: current rotor position (0-25)
// direction: 0=forward, 1=reverse
int encode_through_rotor(int k, int rotor_index, int position, int direction,
                         const EnigmaState *state) {
  int i = mod_positive(k + position);

  if (direction == 0) {
    // Forward: Input -> Wiring
    int wired_char = state->rotors[rotor_index].wiring[i];
    return mod_positive(idx(ALPHABET, wired_char) - position);
  } else {
    // Reverse: Wiring -> Input
    char alphabet_char = ALPHABET[i];
    return mod_positive(idx(state->rotors[rotor_index].wiring, alphabet_char) -
                        position);
  }
}

// Apply plugboard transformation
char apply_plugboard(char c, const char *plugboard) {
  if (*plugboard == '\0') {
    return c; // Empty plugboard, no transformation
  }

  const char *ptr = plugboard;
  while (*ptr) {
    if (c == *ptr && *(ptr + 1)) {
      return *(ptr + 1);
    }
    if (c == *(ptr + 1)) {
      return *ptr;
    }
    ptr += 2;
    // Skip spaces between pairs
    while (*ptr == ' ') {
      ptr++;
    }
  }

  return c; // No swap found
}

// Stepping mechanism (implements the "Double Step" anomaly)
void step_rotors(EnigmaState *state) {
  // Rotor 2 (Middle) steps if it is at notch, moving Rotor 3 (Left)
  if (state->positions[1] == state->notch_positions[1]) {
    state->positions[1] = mod_positive(state->positions[1] + 1);
    state->positions[2] = mod_positive(state->positions[2] + 1);
  }
  // Rotor 2 steps if Rotor 1 (Right) is at notch
  else if (state->positions[0] == state->notch_positions[0]) {
    state->positions[1] = mod_positive(state->positions[1] + 1);
  }
  // Rotor 1 (Right) always steps
  state->positions[0] = mod_positive(state->positions[0] + 1);
}

// Encrypt a single character (returns encrypted char, or original if non-alpha)
// Updates rotor positions as a side effect
int encrypt_char(int c, EnigmaState *state) {
  // Convert lowercase to uppercase
  if (c >= 'a' && c <= 'z') {
    c -= 32;
  }

  // Pass through non-alphabetic characters unchanged
  if (c < 'A' || c > 'Z') {
    return c;
  }

  // STEPPING MECHANISM (The "Double Step" Anomaly)
  step_rotors(state);

  // PLUGBOARD (Input)
  c = apply_plugboard((char)c, state->plugboard);

  // ENCRYPTION PATH
  c = c - 'A'; // Convert to 0-25 range

  // Forward through rotors: Right -> Middle -> Left
  c = encode_through_rotor(c, 2, state->positions[0], 0,
                           state); // Right  (III)
  c = encode_through_rotor(c, 1, state->positions[1], 0,
                           state); // Middle (II)
  c = encode_through_rotor(c, 0, state->positions[2], 0, state); // Left   (I)

  // REFLECTOR B (Fixed)
  c = idx(ALPHABET, state->rotors[3].wiring[c]);

  // Reverse through rotors: Left -> Middle -> Right
  c = encode_through_rotor(c, 0, state->positions[2], 1,
                           state); // Left   (I) Rev
  c = encode_through_rotor(c, 1, state->positions[1], 1,
                           state); // Middle (II) Rev
  c = encode_through_rotor(c, 2, state->positions[0], 1,
                           state); // Right  (III) Rev

  c = c + 'A'; // Convert back to ASCII

  // PLUGBOARD (Output)
  c = apply_plugboard((char)c, state->plugboard);

  return c;
}

// Main encryption loop - line-based for better interactive experience
void run_enigma(EnigmaState *state) {
  char input_line[1024];
  char output_line[1024];

  while (fgets(input_line, sizeof(input_line), stdin) != NULL) {
    int out_idx = 0;

    for (int i = 0; input_line[i] != '\0' && out_idx < 1022; i++) {
      int c = (unsigned char)input_line[i];

      // Skip newline for output, we'll add it at the end
      if (c == '\n' || c == '\r') {
        continue;
      }

      output_line[out_idx++] = (char)encrypt_char(c, state);
    }

    output_line[out_idx] = '\0';
    printf("%s\n", output_line);
    fflush(stdout);
  }
}

// Runtime configuration functions

// Interactive configuration (for teletype/terminal use)
void interactive_config(EnigmaState *state) {
  char input[256];

  printf("UNIGMA: THE LITTLE UNIVAC ENIGMA SIMULATOR\n");
  printf("ROTORS: I, II, III | REFLECTOR: B\n");
  printf("\n");
  printf("--- CONFIGURATION ---\n");
  printf("\n");

  // Get rotor positions
  printf("ROTOR POSITIONS (3 LETTERS A-Z, PRESS ENTER FOR AAA): ");
  fflush(stdout);

  if (fgets(input, sizeof(input), stdin) != NULL) {
    // Remove newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
      input[len - 1] = '\0';
      len--;
    }

    // If empty, use default (already set to AAA)
    if (len > 0) {
      // Trim whitespace
      char *start = input;
      while (*start == ' ' || *start == '\t')
        start++;

      if (*start != '\0') {
        set_rotor_positions(state, start);
        printf("POSITIONS SET TO: %c%c%c\n", 'A' + state->positions[2],
               'A' + state->positions[1], 'A' + state->positions[0]);
      } else {
        printf("USING DEFAULT: AAA\n");
      }
    } else {
      printf("USING DEFAULT: AAA\n");
    }
  }

  printf("\n");

  // Get plugboard configuration
  printf("PLUGBOARD PAIRS (E.G. 'AB CD EF', PRESS ENTER FOR NONE): ");
  fflush(stdout);

  if (fgets(input, sizeof(input), stdin) != NULL) {
    // Remove newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
      input[len - 1] = '\0';
      len--;
    }

    // Trim whitespace
    char *start = input;
    while (*start == ' ' || *start == '\t')
      start++;

    if (*start != '\0') {
      set_plugboard(state, start);
      printf("PLUGBOARD SET TO: %s\n", state->plugboard);
    } else {
      printf("NO PLUGBOARD\n");
    }
  }

  printf("\n");
  printf("--- READY TO ENCRYPT/DECRYPT ---\n");
  printf("ENTER TEXT (CTRL+Z OR CTRL+D TO END):\n");
  printf("\n");
  fflush(stdout);
}

// Print usage information
void print_usage(const char *program_name) {
  fprintf(stderr, "UNIGMA: THE LITTLE UNIVAC ENIGMA SIMULATOR\n");
  fprintf(stderr, "USAGE: %s [OPTIONS]\n\n", program_name);
  fprintf(stderr, "OPTIONS:\n");
  fprintf(
      stderr,
      "  -P POSITIONS    SET ROTOR POSITIONS (3 LETTERS A-Z, DEFAULT: AAA)\n");
  fprintf(stderr, "                  EXAMPLE: -P XYZ\n");
  fprintf(stderr,
          "  -B PLUGBOARD    SET PLUGBOARD PAIRS (SPACE-SEPARATED PAIRS)\n");
  fprintf(stderr, "                  EXAMPLE: -B \"AB CD EF\"\n");
  fprintf(stderr, "  -S              SHOW CURRENT CONFIGURATION AND EXIT\n");
  fprintf(stderr, "  -H              SHOW THIS HELP MESSAGE\n\n");
  fprintf(stderr, "EXAMPLES:\n");
  fprintf(stderr, "  %s -P AAA                    # START AT POSITION AAA\n",
          program_name);
  fprintf(stderr,
          "  %s -P XYZ -B \"AB CD\"         # CUSTOM POSITION AND PLUGBOARD\n",
          program_name);
  fprintf(stderr,
          "  ECHO \"HELLO\" | %s -P QWE    # ENCRYPT WITH POSITION QWE\n\n",
          program_name);
  fprintf(stderr, "ROTORS: I, II, III | REFLECTOR: B\n");
}

// Print current configuration
void print_current_config(const EnigmaState *state) {
  char pos_str[4];
  pos_str[0] = 'A' + state->positions[2]; // Left rotor
  pos_str[1] = 'A' + state->positions[1]; // Middle rotor
  pos_str[2] = 'A' + state->positions[0]; // Right rotor
  pos_str[3] = '\0';

  fprintf(stderr, "=== ENIGMA CONFIGURATION ===\n");
  fprintf(stderr, "ROTORS:     I, II, III\n");
  fprintf(stderr, "REFLECTOR:  B\n");
  fprintf(stderr, "POSITIONS:  %s (LEFT: %c, MIDDLE: %c, RIGHT: %c)\n", pos_str,
          pos_str[0], pos_str[1], pos_str[2]);
  fprintf(stderr, "PLUGBOARD:  %s\n",
          state->plugboard[0] ? state->plugboard : "(NONE)");
  fprintf(stderr, "============================\n");
}

// Set rotor positions from string (e.g., "ABC" or "XYZ")
void set_rotor_positions(EnigmaState *state, const char *positions) {
  if (!positions || strlen(positions) != 3) {
    fprintf(stderr, "ERROR: ROTOR POSITIONS MUST BE EXACTLY 3 LETTERS (A-Z)\n");
    exit(1);
  }

  for (int i = 0; i < 3; i++) {
    char c = positions[i];
    if (c >= 'a' && c <= 'z') {
      c -= 32; // Convert to uppercase
    }
    if (c < 'A' || c > 'Z') {
      fprintf(stderr, "ERROR: INVALID ROTOR POSITION '%c'. MUST BE A-Z.\n",
              positions[i]);
      exit(1);
    }
  }

  // Positions are specified as Left-Middle-Right, but stored as
  // Right-Middle-Left
  state->positions[2] = positions[0] - 'A'; // Left rotor
  state->positions[1] = positions[1] - 'A'; // Middle rotor
  state->positions[0] = positions[2] - 'A'; // Right rotor
}

// Set plugboard configuration
void set_plugboard(EnigmaState *state, const char *plugboard_config) {
  if (!plugboard_config) {
    state->plugboard[0] = '\0';
    return;
  }

  size_t len = strlen(plugboard_config);
  if (len >= MAX_PLUGBOARD_LEN) {
    fprintf(stderr,
            "ERROR: PLUGBOARD CONFIGURATION TOO LONG (MAX %d CHARACTERS)\n",
            MAX_PLUGBOARD_LEN - 1);
    exit(1);
  }

  SAFE_STRCPY(state->plugboard, plugboard_config, MAX_PLUGBOARD_LEN);

  // Convert to uppercase
  for (size_t i = 0; i < strlen(state->plugboard); i++) {
    if (state->plugboard[i] >= 'a' && state->plugboard[i] <= 'z') {
      state->plugboard[i] -= 32;
    }
  }
}

// Parse command-line arguments
void parse_arguments(int argc, char *argv[], EnigmaState *state) {
  int show_config = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-H") == 0) {
      print_usage(argv[0]);
      exit(0);
    } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-S") == 0) {
      show_config = 1;
    } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "-P") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "ERROR: -P REQUIRES AN ARGUMENT (3 LETTERS A-Z)\n");
        print_usage(argv[0]);
        exit(1);
      }
      set_rotor_positions(state, argv[++i]);
    } else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "-B") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "ERROR: -B REQUIRES AN ARGUMENT (PLUGBOARD PAIRS)\n");
        print_usage(argv[0]);
        exit(1);
      }
      set_plugboard(state, argv[++i]);
    } else {
      fprintf(stderr, "ERROR: UNKNOWN OPTION '%s'\n\n", argv[i]);
      print_usage(argv[0]);
      exit(1);
    }
  }

  if (show_config) {
    print_current_config(state);
    exit(0);
  }
}

// Platform-specific console setup (Windows only)
#ifndef UNIVAC
void console_setup(void) {
  // Set console to UTF-8 for better text handling
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
}
#endif
