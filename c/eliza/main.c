/*
 * ELIZA/DOCTOR Simulation
 * Main implementation
 * Created by Joseph Weizenbaum
 * This version by Jeff Shrager
 * Edited and modified for MITS 8K BASIC 4.0 by Steve North
 * From Creative Computing Jul-Aug 1977
 */

#include "ELIZA.h"

// Platform-specific string copy
#ifdef UNIVAC
#define SAFE_STRCPY(dest, src, size) do { strncpy(dest, src, (size)-1); (dest)[(size)-1] = '\0'; } while(0)
#else
#define SAFE_STRCPY(dest, src, size) strcpy_s(dest, size, src)
#endif

// Keywords (matches DATA lines 1050-1100)
const char* keywords[NUM_KEYWORDS] = {
    "CAN YOU", "CAN I", "YOU ARE", "YOURE", "I DONT", "I FEEL",
    "WHY DONT YOU", "WHY CANT I", "ARE YOU", "I CANT", "I AM", "IM ",
    "YOU ", "I WANT", "WHAT", "HOW", "WHO", "WHERE", "WHEN", "WHY",
    "NAME", "CAUSE", "SORRY", "DREAM", "HELLO", "HI ", "MAYBE",
    " NO", "YOUR", "ALWAYS", "THINK", "ALIKE", "YES", "FRIEND",
    "COMPUTER", "NOKEYFOUND"
};

// Conjugation pairs (matches DATA lines 1230-1235)
const ConjugationPair conjugations[NUM_CONJUGATIONS / 2] = {
    {" ARE ", " AM "}, {"WERE ", "WAS "}, {" YOU ", " I "}, 
    {"YOUR ", "MY "}, {" IVE ", " YOUVE "}, {" IM ", " YOURE "}
};

// Replies (matches DATA lines 1330-2430)
const char* replies[NUM_REPLIES] = {
    "DON'T YOU BELIEVE THAT I CAN*",
    "PERHAPS YOU WOULD LIKE TO BE ABLE TO*",
    "YOU WANT ME TO BE ABLE TO*",
    "PERHAPS YOU DON'T WANT TO*",
    "DO YOU WANT TO BE ABLE TO*",
    "WHAT MAKES YOU THINK I AM*",
    "DOES IT PLEASE YOU TO BELIEVE I AM*",
    "PERHAPS YOU WOULD LIKE TO BE*",
    "DO YOU SOMETIMES WISH YOU WERE*",
    "DON'T YOU REALLY*",
    "WHY DON'T YOU*",
    "DO YOU WISH TO BE ABLE TO*",
    "DOES THAT TROUBLE YOU?",
    "TELL ME MORE ABOUT SUCH FEELINGS.",
    "DO YOU OFTEN FEEL*",
    "DO YOU ENJOY FEELING*",
    "DO YOU REALLY BELIEVE I DON'T*",
    "PERHAPS IN GOOD TIME I WILL*",
    "DO YOU WANT ME TO*",
    "DO YOU THINK YOU SHOULD BE ABLE TO*",
    "WHY CAN'T YOU*",
    "WHY ARE YOU INTERESTED IN WHETHER OR NOT I AM*",
    "WOULD YOU PREFER IF I WERE NOT*",
    "PERHAPS IN YOUR FANTASIES I AM*",
    "HOW DO YOU KNOW YOU CAN'T*",
    "HAVE YOU TRIED?",
    "PERHAPS YOU CAN NOW*",
    "DID YOU COME TO ME BECAUSE YOU ARE*",
    "HOW LONG HAVE YOU BEEN*",
    "DO YOU BELIEVE IT IS NORMAL TO BE*",
    "DO YOU ENJOY BEING*",
    "WE WERE DISCUSSING YOU-- NOT ME.",
    "OH, I*",
    "YOU'RE NOT REALLY TALKING ABOUT ME, ARE YOU?",
    "WHAT WOULD IT MEAN TO YOU IF YOU GOT*",
    "WHY DO YOU WANT*",
    "SUPPOSE YOU SOON GOT*",
    "WHAT IF YOU NEVER GOT*",
    "I SOMETIMES ALSO WANT*",
    "WHY DO YOU ASK?",
    "DOES THAT QUESTION INTEREST YOU?",
    "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
    "WHAT DO YOU THINK?",
    "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
    "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
    "HAVE YOU ASKED ANYONE ELSE?",
    "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
    "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?",
    "NAMES DON'T INTEREST ME.",
    "I DON'T CARE ABOUT NAMES-- PLEASE GO ON.",
    "IS THAT THE REAL REASON?",
    "DON'T ANY OTHER REASONS COME TO MIND?",
    "DOES THAT REASON EXPLAIN ANYTHING ELSE?",
    "WHAT OTHER REASONS MIGHT THERE BE?",
    "PLEASE DON'T APOLOGIZE!",
    "APOLOGIES ARE NOT NECESSARY.",
    "WHAT FEELINGS DO YOU HAVE WHEN YOU APOLOGIZE.",
    "DON'T BE SO DEFENSIVE!",
    "WHAT DOES THAT DREAM SUGGEST TO YOU?",
    "DO YOU DREAM OFTEN?",
    "WHAT PERSONS APPEAR IN YOUR DREAMS?",
    "ARE YOU DISTURBED BY YOUR DREAMS?",
    "HOW DO YOU DO ... PLEASE STATE YOUR PROBLEM.",
    "YOU DON'T SEEM QUITE CERTAIN.",
    "WHY THE UNCERTAIN TONE?",
    "CAN'T YOU BE MORE POSITIVE?",
    "YOU AREN'T SURE?",
    "DON'T YOU KNOW?",
    "ARE YOU SAYING NO JUST TO BE NEGATIVE?",
    "YOU ARE BEING A BIT NEGATIVE.",
    "WHY NOT?",
    "ARE YOU SURE?",
    "WHY NO?",
    "WHY ARE YOU CONCERNED ABOUT MY*",
    "WHAT ABOUT YOUR OWN*",
    "CAN YOU THINK OF A SPECIFIC EXAMPLE?",
    "WHEN?",
    "WHAT ARE YOU THINKING OF?",
    "REALLY, ALWAYS?",
    "DO YOU REALLY THINK SO?",
    "BUT YOU ARE NOT SURE YOU*",
    "DO YOU DOUBT YOU*",
    "IN WHAT WAY?",
    "WHAT RESEMBLANCE DO YOU SEE?",
    "WHAT DOES THE SIMILARITY SUGGEST TO YOU?",
    "WHAT OTHER CONNECTIONS DO YOU SEE?",
    "COULD THERE REALLY BE SOME CONNECTION?",
    "HOW?",
    "YOU SEEM QUITE POSITIVE.",
    "ARE YOU SURE?",
    "I SEE.",
    "I UNDERSTAND.",
    "WHY DO YOU BRING UP THE TOPIC OF FRIENDS?",
    "DO YOUR FRIENDS WORRY YOU?",
    "DO YOUR FRIENDS PICK ON YOU?",
    "ARE YOU SURE YOU HAVE ANY FRIENDS?",
    "DO YOU IMPOSE ON YOUR FRIENDS?",
    "PERHAPS YOUR LOVE FOR FRIENDS WORRIES YOU.",
    "DO COMPUTERS WORRY YOU?",
    "ARE YOU TALKING ABOUT ME IN PARTICULAR?",
    "ARE YOU FRIGHTENED BY MACHINES?",
    "WHY DO YOU MENTION COMPUTERS?",
    "WHAT DO YOU THINK MACHINES HAVE TO DO WITH YOUR PROBLEM?",
    "DON'T YOU THINK COMPUTERS CAN HELP PEOPLE?",
    "WHAT IS IT ABOUT MACHINES THAT WORRIES YOU?",
    "SAY, DO YOU HAVE ANY PSYCHOLOGICAL PROBLEMS?",
    "WHAT DOES THAT SUGGEST TO YOU?",
    "I SEE.",
    "I'M NOT SURE I UNDERSTAND YOU FULLY.",
    "COME COME ELUCIDATE YOUR THOUGHTS.",
    "CAN YOU ELABORATE ON THAT?",
    "THAT IS QUITE INTERESTING."
};

// Reply range initialization (matches DATA lines 2530-2560)
const int reply_init_data[NUM_KEYWORDS * 2] = {
    1,3, 4,2, 6,4, 6,4, 10,4, 14,3, 17,3, 20,2, 22,3, 25,3,
    28,4, 28,4, 32,3, 35,5, 40,9, 40,9, 40,9, 40,9, 40,9, 40,9,
    49,2, 51,4, 55,4, 59,4, 63,1, 63,1, 64,5, 69,5, 74,2, 76,4,
    80,3, 83,7, 90,3, 93,6, 99,7, 106,6
};

// Main entry point
int main(void) {
#ifndef UNIVAC
    console_setup();
#endif
    
    ElizaState state;
    init_eliza(&state);
    run_eliza(&state);
    
    return 0;
}

// Initialize ELIZA state
void init_eliza(ElizaState* state) {
    memset(state, 0, sizeof(ElizaState));
    init_reply_ranges(state);
}

// Initialize reply ranges (matches lines 130-150)
void init_reply_ranges(ElizaState* state) {
    for (int x = 0; x < NUM_KEYWORDS; x++) {
        int start_idx = reply_init_data[x * 2] - 1;     // Convert to 0-based index
        int count = reply_init_data[x * 2 + 1];
        
        state->reply_ranges[x].start = start_idx;
        state->reply_ranges[x].count = count;
        state->reply_ranges[x].current = start_idx;
        state->reply_ranges[x].end = start_idx + count - 1;
    }
}

// Main conversation loop
void run_eliza(ElizaState* state) {
    printf("HI!  I'M ELIZA.  WHAT'S YOUR PROBLEM?\n");
    
    while (1) {
        get_user_input(state);
        
        // Check for repetition (line 255)
        if (strcmp(state->input, state->previous) == 0) {
            printf("PLEASE DON'T REPEAT YOURSELF!\n");
            continue;
        }
        
        // Find keyword in input
        int keyword_found = find_keyword(state);
        
        if (keyword_found) {
            // Conjugate the string after the keyword
            conjugate_string(state);
        } else {
            // No keyword found, use default responses
            state->keyword_index = KW_NOKEYFOUND;
        }
        
        // Generate and print reply
        generate_reply(state);
        
        // Save current input as previous
        SAFE_STRCPY(state->previous, state->input, sizeof(state->previous));
    }
}

// Get user input and process it (lines 200-250)
void get_user_input(ElizaState* state) {
    char raw_input[MAX_INPUT_LEN];
    fgets(raw_input, sizeof(raw_input), stdin);
    
    // Remove newline
    size_t len = strlen(raw_input);
    if (len > 0 && raw_input[len - 1] == '\n') {
        raw_input[len - 1] = '\0';
    }
    
    // Convert to uppercase
    to_uppercase(raw_input);
    
    // Add spaces at beginning and end (line 201)
    snprintf(state->input, sizeof(state->input), " %s  ", raw_input);
    
    // Remove apostrophes (lines 220-230)
    remove_apostrophes(state->input);
    
    // Check for quit command (lines 240-250)
    check_for_quit(state->input);
}

// Remove apostrophes from string (lines 220-230)
void remove_apostrophes(char* str) {
    char* src = str;
    char* dst = str;
    
    while (*src) {
        if (*src != '\'') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

// Check for quit command (lines 240-250)
void check_for_quit(const char* str) {
    if (strstr(str, "SHUT") != NULL) {
        printf("SHUT UP...\n");
        exit(0);
    }
}

// Find keyword in input string (lines 290-370)
int find_keyword(ElizaState* state) {
    int found_keyword = -1;
    int found_position = -1;
    
    // Search for keywords in priority order
    for (int k = 0; k < NUM_KEYWORDS - 1; k++) {  // Exclude NOKEYFOUND
        const char* keyword = keywords[k];
        int keyword_len = (int)strlen(keyword);
        
        // Search for this keyword in the input
        for (int pos = 0; pos <= (int)strlen(state->input) - keyword_len; pos++) {
            if (strncmp(&state->input[pos], keyword, keyword_len) == 0) {
                found_keyword = k;
                found_position = pos;
                SAFE_STRCPY(state->keyword, keyword, sizeof(state->keyword));
                break;
            }
        }
        
        if (found_keyword >= 0) break;
    }
    
    if (found_keyword >= 0) {
        state->keyword_index = found_keyword;
        state->keyword_pos = found_position;
        return 1;
    }
    
    return 0;
}

// Conjugate string after the keyword (lines 420-560)
void conjugate_string(ElizaState* state) {
    // Get the part after the keyword
    int keyword_len = (int)strlen(state->keyword);
    int start_pos = state->keyword_pos + keyword_len;
    
    // Copy the remainder with a leading space
    snprintf(state->conjugated, sizeof(state->conjugated), " %s", 
             &state->input[start_pos]);
    
    // Apply conjugations
    for (int i = 0; i < NUM_CONJUGATIONS / 2; i++) {
        // Apply both directions of conjugation
        swap_words(state->conjugated, conjugations[i].from, conjugations[i].to);
        swap_words(state->conjugated, conjugations[i].to, conjugations[i].from);
    }
    
    // Remove leading space if there's only one space (line 555)
    if (strlen(state->conjugated) >= 2 && state->conjugated[1] == ' ') {
        memmove(state->conjugated, &state->conjugated[1], strlen(state->conjugated));
    }
}

// Swap words in a string (similar to lines 460-540)
void swap_words(char* str, const char* from, const char* to) {
    char temp[MAX_STRING_LEN];
    char* pos;
    
    while ((pos = strstr(str, from)) != NULL) {
        // Calculate positions
        size_t prefix_len = pos - str;
        size_t from_len = strlen(from);
        
        // Build new string
        strncpy(temp, str, prefix_len);
        temp[prefix_len] = '\0';
        strcat(temp, to);
        strcat(temp, pos + from_len);
        
        // Copy back
        SAFE_STRCPY(str, temp, MAX_STRING_LEN);
    }
}

// Generate reply (lines 590-640)
void generate_reply(ElizaState* state) {
    const char* reply_template = get_next_reply(state, state->keyword_index);
    
    // Check if reply ends with '*' (indicating it needs conjugated text)
    size_t reply_len = strlen(reply_template);
    if (reply_len > 0 && reply_template[reply_len - 1] == '*') {
        // Print reply without the '*' and append conjugated text
        printf("%.*s%s\n", (int)(reply_len - 1), reply_template, state->conjugated);
    } else {
        // Print reply as-is
        printf("%s\n", reply_template);
    }
}

// Get next reply for a keyword (lines 600-610)
const char* get_next_reply(ElizaState* state, int keyword_idx) {
    ReplyRange* range = &state->reply_ranges[keyword_idx];
    
    // Get current reply
    const char* reply = replies[range->current];
    
    // Advance to next reply (cycling)
    range->current++;
    if (range->current > range->end) {
        range->current = range->start;
    }
    
    return reply;
}

// Convert string to uppercase
void to_uppercase(char* str) {
    while (*str) {
        *str = (char)toupper(*str);
        str++;
    }
}

// Trim whitespace from string
void trim_string(char* str) {
    // Trim leading whitespace
    char* start = str;
    while (isspace(*start)) start++;
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        *end = '\0';
        end--;
    }
}

// Find substring in string
int str_find(const char* haystack, const char* needle) {
    const char* pos = strstr(haystack, needle);
    return pos ? (int)(pos - haystack) : -1;
}

// Replace substring at position
void str_replace_at(char* dest, size_t dest_size, int pos, int len, const char* replacement) {
    char temp[MAX_STRING_LEN];
    
    // Copy prefix
    strncpy(temp, dest, pos);
    temp[pos] = '\0';
    
    // Add replacement
    strncat(temp, replacement, dest_size - strlen(temp) - 1);
    
    // Add suffix
    strncat(temp, dest + pos + len, dest_size - strlen(temp) - 1);
    
    // Copy back
    SAFE_STRCPY(dest, temp, dest_size);
}

// Platform-specific console setup
#ifndef UNIVAC
void console_setup(void) {
    // Set console to UTF-8 for better text handling
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}
#endif
