/*
 * BBC Baseball Simulation and Demonstrator (1961)
 * Modern C implementation for Windows Console
 * Original by Paul R. Burgeson
 *
 * Rewritten for Windows conhost terminal
 * Compiles with MSVC
 */

#ifndef BASEBALL_H
#define BASEBALL_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Constants
#define MAX_PLAYERS 100
#define TEAM_SIZE 9
#define MAX_NAME_LEN 30
#define MAX_TEAM_NAME_LEN 20
#define MAX_INNINGS 9

// Player handedness
typedef enum { HAND_RIGHT = 'R', HAND_LEFT = 'L', HAND_BOTH = 'B' } Handedness;

// Player positions (for roster organization)
typedef enum {
  POS_FIRST_BASE = 0,
  POS_SECOND_BASE = 10,
  POS_THIRD_BASE = 20,
  POS_SHORTSTOP = 30,
  POS_LEFT_FIELD = 40,
  POS_CENTER_FIELD = 50,
  POS_RIGHT_FIELD = 60,
  POS_CATCHER = 70,
  POS_PITCHER = 80
} Position;

// Player structure
typedef struct {
  char name[MAX_NAME_LEN];
  int year;
  char team[MAX_TEAM_NAME_LEN];
  int batting_avg; // stored as integer (e.g., 393 for .393)
  Handedness hand;
  int position;
  int j_num; // Jersey number
} Player;

// Game state structure
typedef struct {
  Player *visiting_team[TEAM_SIZE];
  Player *home_team[TEAM_SIZE];
  int visiting_score;
  int home_score;
  int inning;
  int is_bottom; // 0 = top, 1 = bottom
  int outs;
  int bases[3]; // 0 = empty, 1 = runner on base
  int visiting_hits;
  int home_hits;
  int visiting_errors;
  int home_errors;
  int current_batter;
  unsigned int rand_seed1;
  unsigned int rand_seed2;
  // Track half-inning stats
  int half_inning_runs;
  int half_inning_hits;
  int half_inning_errors;
} GameState;

// Play result types
typedef enum {
  PLAY_SINGLE,
  PLAY_DOUBLE,
  PLAY_TRIPLE,
  PLAY_HOME_RUN,
  PLAY_GROUND_OUT,
  PLAY_FLY_OUT,
  PLAY_LINE_OUT,
  PLAY_STRIKEOUT_SWINGING,
  PLAY_STRIKEOUT_CALLED,
  PLAY_WALK,
  PLAY_ERROR,
  PLAY_DOUBLE_PLAY,
  PLAY_TRIPLE_PLAY,
  PLAY_FIELDERS_CHOICE,
  PLAY_SACRIFICE_FLY
} PlayType;

// Function declarations

// Player database
extern Player player_roster[MAX_PLAYERS];

// Random number generation
void init_random(const char *date_str, const char *time_str);
int random_number(int min, int max);
int weighted_random(int batting_avg);

// Game initialization
void init_game(GameState *game);
void select_visiting_team(GameState *game);
void select_home_team(GameState *game);

// Game simulation
void play_game(GameState *game);
void play_at_bat(GameState *game, Player *batter);
PlayType determine_play_result(Player *batter, GameState *game);
void execute_play(GameState *game, PlayType play, Player *batter);
void advance_runners(GameState *game, int bases_to_advance);
int calculate_runs_scored(GameState *game, int bases_to_advance);

// Output functions
void print_header(void);
void print_lineup(const char *team_name, Player *team[]);
void print_play_result(Player *batter, PlayType play, int runs_scored,
                       GameState *game);
void print_inning_summary(GameState *game);
void print_final_score(GameState *game);
void print_base_situation(GameState *game);

// Utility functions
Player *find_player(const char *name);
void clear_bases(GameState *game);
void to_uppercase(char *str);
void console_setup(void);

#endif // BASEBALL_H