/*
 * Oregon Trail Simulation (1847)
 * Modern C implementation for Windows Console
 * Original BASIC programming by Bill Heinemann - 1971
 * Support research and materials by Don Rawitsch,
 * Minnesota Educational Computing Consortium Staff
 * 
 * Rewritten for Windows conhost terminal
 * Compiles with MSVC and MinGW
 */

#ifndef OREGON_H
#define OREGON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

// Platform-specific includes
#ifndef UNIVAC
#include <windows.h>
#endif

// Constants
#define MAX_INPUT_LEN 50
#define TOTAL_DISTANCE 2040
#define STARTING_MONEY 700
#define WAGON_COST 200
#define AVAILABLE_MONEY (STARTING_MONEY - WAGON_COST + 200)  // 700 total available

// Game state flags
#define FLAG_ILLNESS 1
#define FLAG_INJURY 2
#define FLAG_SOUTH_PASS 4
#define FLAG_BLUE_MOUNTAINS 8
#define FLAG_BLIZZARD 16
#define FLAG_FORT_OPTION 32

// Event probabilities (stored in data array)
extern const int event_probabilities[15];

// Game state structure
typedef struct {
    // Resources
    int food;          // Food supplies
    int bullets;       // Ammunition (in individual bullets)
    int clothing;      // Clothing supplies
    int misc_supplies; // Medical and misc supplies (M1 in original)
    int cash;          // Remaining money
    int oxen_cost;     // Amount spent on oxen (affects speed)
    
    // Progress
    int miles_traveled;      // Total miles traveled (M in original)
    int miles_previous_turn; // Miles at start of current turn (M2 in original)
    int turn_number;         // Turn counter for date calculation (D3 in original)
    
    // Player stats
    int shooting_skill; // 1-5, where 1 is best (D9 in original)
    int eating_level;   // 1-3, where 3 is eating well (E in original)
    
    // Flags and state
    int game_flags;     // Bit flags for various states
    int fort_available; // Whether fort option is available this turn (X1 in original)
    
    // Random state for consistent gameplay
    unsigned int rand_seed;
} GameState;

// Event types
typedef enum {
    EVENT_WAGON_BREAKDOWN = 0,
    EVENT_OX_INJURY,
    EVENT_DAUGHTER_BREAKS_ARM,
    EVENT_OX_WANDERS_OFF,
    EVENT_SON_GETS_LOST,
    EVENT_UNSAFE_WATER,
    EVENT_HEAVY_RAINS,
    EVENT_BANDITS_ATTACK,
    EVENT_FIRE_IN_WAGON,
    EVENT_LOSE_WAY_IN_FOG,
    EVENT_POISONOUS_SNAKE,
    EVENT_WAGON_SWAMPED_FORDING,
    EVENT_WILD_ANIMALS_ATTACK,
    EVENT_COLD_WEATHER,
    EVENT_HAIL_STORM,
    EVENT_HELPFUL_INDIANS
} EventType;

// Death causes
typedef enum {
    DEATH_STARVATION,
    DEATH_EXHAUSTION,
    DEATH_DISEASE,
    DEATH_INJURIES,
    DEATH_WINTER_BLIZZARD,
    DEATH_SNAKEBITE,
    DEATH_MASSACRE
} DeathCause;

// Function declarations

// Game initialization and main loop
void init_game(GameState* game);
void show_instructions(void);
void setup_initial_purchases(GameState* game);
void main_game_loop(GameState* game);

// Date and time functions
void print_current_date(int turn_number);
const char* get_date_string(int turn_number);

// Turn mechanics
void process_turn(GameState* game);
void display_status(GameState* game);
int get_user_choice(const char* prompt, int min_choice, int max_choice);
void handle_turn_choice(GameState* game, int choice);

// Fort interactions
void visit_fort(GameState* game);
int get_purchase_amount(const char* item_name, int max_money);

// Hunting
void go_hunting(GameState* game);
int shooting_minigame(int skill_level);
void print_shooting_word(char* word_buffer);

// Travel and events
void travel_segment(GameState* game);
void check_for_riders(GameState* game);
void handle_rider_encounter(GameState* game, int hostile);
void process_random_events(GameState* game);
void handle_event(GameState* game, EventType event);

// Mountain travel
void mountain_travel(GameState* game);
void check_mountain_events(GameState* game);

// Health and survival
void check_eating_and_health(GameState* game);
void handle_illness(GameState* game);
void check_starvation(GameState* game);
void check_winter_arrival(GameState* game);

// Resource management
void consume_food(GameState* game);
void validate_resources(GameState* game);
void pay_doctor_bill(GameState* game);

// End game conditions
void check_victory_condition(GameState* game);
void handle_death(GameState* game, DeathCause cause);
void show_death_scene(DeathCause cause);
void show_victory_scene(GameState* game);
void calculate_final_date(GameState* game);

// Random number generation
void init_random(void);
int random_int(int min, int max);
double random_double(void);

// Utility functions
void clear_input_buffer(void);
void wait_for_keypress(void);
void print_separator(void);
int get_yes_no_input(const char* prompt);
void to_uppercase(char* str);

// Platform-specific functions
#ifndef UNIVAC
void console_setup(void);
#endif

#endif // OREGON_H
