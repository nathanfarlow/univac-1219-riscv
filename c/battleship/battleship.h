/*
 * battleship.h - Cross-platform Battleship Game Header
 * Compatible with: MSVC, MinGW (Windows GCC), UNIVAC 1219 (GCC)
 */

#ifndef BATTLESHIP_H
#define BATTLESHIP_H

/* Platform detection and includes */
#ifdef UNIVAC
    #define PLATFORM_NAME "UNIVAC 1219"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>
#else
    #define PLATFORM_NAME "WINDOWS"
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>
    #ifndef UNIVAC
        #ifdef _MSC_VER
            #include <windows.h>
        #endif
    #endif
#endif

/* Constants */
#define BOARD_SIZE 10
#define NO_OF_SHIPS 5
#define MAX_SHIP_LENGTH 5
#define MAX_NAME_LENGTH 50
#define MAX_COORD_LENGTH 10
#define MAX_POSITIONS 100

/* Ship types */
typedef struct {
    char name[MAX_NAME_LENGTH];
    int length;
    char positions[MAX_SHIP_LENGTH][MAX_COORD_LENGTH];
    int position_count;
} Ship;

/* Battlefield piece types */
#define WATER '~'
#define HIT 'X'
#define MISS 'M'
#define SHIP_PIECE '0'

/* Battlefield validation codes */
#define VALID_COORD 0x0F
#define TOUCHING 0x1C
#define CROSSING 0xAF
#define OUT_OF_BOARD 0xBD
#define WRONG_LENGTH 0xFF
#define MISALIGN 0x4E

/* Battlefield structure */
typedef struct {
    char board[BOARD_SIZE][BOARD_SIZE];
} Battlefield;

/* Player structure */
typedef struct {
    char name[MAX_NAME_LENGTH];
    Battlefield arena;
    Ship ships[NO_OF_SHIPS];
    int ship_count;
} Player;

/* AI Engine structures */
typedef struct {
    int targets[MAX_POSITIONS];
    int target_count;
    int hunts[MAX_POSITIONS];
    int hunt_count;
    int targets_fired[MAX_POSITIONS];
    int targets_fired_count;
    int is_targeting;
    char previous_shot[MAX_COORD_LENGTH];
} IntermediateAI;

/* Random number generator state for UNIVAC */
#ifdef UNIVAC
typedef struct {
    unsigned int state;
} XorShiftRNG;
#endif

/* Function prototypes - Battlefield */
void init_battlefield(Battlefield* bf);
void print_battlefield(Battlefield* bf, int is_wartime);
int is_hit(Battlefield* bf, char row, int col);
int is_miss(Battlefield* bf, char row, int col);
void place_piece(Battlefield* bf, char row, int col, char piece);
char get_piece(Battlefield* bf, char row, int col);
int is_correct_coordinates(Battlefield* bf, char roF, char roS, int coF, int coS, Ship* s);
int is_crossing(Battlefield* bf, char roF, char roS, int coF, int coS);
int is_touching(Battlefield* bf, char roF, char roS, int coF, int coS);

/* Function prototypes - Ship */
void init_ship(Ship* s, const char* name, int length);
void store_ship_placement(Ship* s, char roF, char roS, int coF, int coS);
int is_part_of_ship(Ship* s, char row, int col);
void remove_ship_part(Ship* s, char row, int col);
int is_ship_sunken(Ship* s);

/* Function prototypes - Player */
void init_player(Player* p, const char* name);
int is_navy_sunken(Player* p);
void manage_ship_hit(Player* p, char row, int col);

/* Function prototypes - AI Engine */
void init_intermediate_ai(IntermediateAI* ai);
void ai_place_ship(Player* p, int ship_index, unsigned int* rng_state);
void ai_fire_salvo(IntermediateAI* ai, char* result, unsigned int* rng_state);
void ai_manage_ship_hit(Player* p, IntermediateAI* ai, char row, int col);
int encode_coord(const char* coord);
void decode_coord(int encoded, char* result);
void create_targets(IntermediateAI* ai);
void hunt_squares(IntermediateAI* ai, char* result, unsigned int* rng_state);
void target_ship(IntermediateAI* ai, const char* previous_shot, int is_hit, char* result);

/* Function prototypes - Utility */
void clear_screen(void);
void prompt_enter_key(void);
void print_divider(void);
void get_input(char* buffer, int size);
void normalize_coordinates(char* roF, char* roS, int* coF, int* coS);
unsigned int xorshift32(unsigned int* state);
void init_random(unsigned int* state);
int random_range(unsigned int* state, int min, int max);
char random_row(unsigned int* state);
int random_col(unsigned int* state);

/* Platform-specific string functions */
#ifdef _MSC_VER
    #define SAFE_STRCPY(dest, src, size) strcpy_s(dest, size, src)
    #define SAFE_STRCAT(dest, src, size) strcat_s(dest, size, src)
    #define SAFE_SPRINTF(dest, size, fmt, ...) sprintf_s(dest, size, fmt, __VA_ARGS__)
#else
    #define SAFE_STRCPY(dest, src, size) do { \
        strncpy(dest, src, (size) - 1); \
        (dest)[(size) - 1] = '\0'; \
    } while(0)

    #define SAFE_STRCAT(dest, src, size) do { \
        strncat(dest, src, (size) - strlen(dest) - 1); \
    } while(0)

    #define SAFE_SPRINTF(dest, size, fmt, ...) do { \
        snprintf(dest, size, fmt, __VA_ARGS__); \
    } while(0)
#endif

#endif /* BATTLESHIP_H */
