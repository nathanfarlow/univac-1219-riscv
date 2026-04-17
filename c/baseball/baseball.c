/*
 * BBC Baseball Simulation and Demonstrator
 * Main game implementation
 */

#include "baseball.h"

// Global random state
static unsigned int g_rand_seed1 = 0;
static unsigned int g_rand_seed2 = 0;

// Position names for output
static const char *position_names[] = {"FIRST", "SECOND",  "THIRD",
                                       "SHORT", "LEFT",    "CENTER",
                                       "RIGHT", "CATCHER", "PITCHER"};

static const char *field_locations[] = {
    "LEFT",  "CENTER", "RIGHT", "L CENTR", "R CENTR",
    "FIRST", "SECOND", "THIRD", "SHORT",   "PITCHER"};

// Initialize random number generators
void init_random(const char *date_str, const char *time_str) {
  g_rand_seed1 = 12345; // Fixed seed for embedded platform
  g_rand_seed2 = g_rand_seed1 ^ 0x12345678;

  // Mix in characters from date and time strings
  for (int i = 0; date_str[i]; i++) {
    g_rand_seed1 = (g_rand_seed1 * 31) + date_str[i];
  }
  for (int i = 0; time_str[i]; i++) {
    g_rand_seed2 = (g_rand_seed2 * 37) + time_str[i];
  }
}

// Random number generation using two separate generators
int random_number(int min, int max) {
  g_rand_seed1 = (g_rand_seed1 * 1103515245 + 12345) & 0x7fffffff;
  g_rand_seed2 = (g_rand_seed2 * 69069 + 1) & 0x7fffffff;

  unsigned int combined = g_rand_seed1 ^ g_rand_seed2;
  return min + (combined % (max - min + 1));
}

// Weighted random based on batting average
int weighted_random(int batting_avg) {
  int roll = random_number(1, 1000);
  // Higher batting average = more likely to get a hit
  return roll <= batting_avg;
}

// Console setup (portable version)
void console_setup(void) {
  // No-op for RISC-V platform
}

// Convert string to uppercase
void to_uppercase(char *str) {
  for (int i = 0; str[i]; i++) {
    str[i] = (char)toupper((unsigned char)str[i]);
  }
}

// Find player by name in roster
Player *find_player(const char *name) {
  char search_name[MAX_NAME_LEN];
  strncpy(search_name, name, sizeof(search_name) - 1);
  search_name[sizeof(search_name) - 1] = '\0';
  to_uppercase(search_name);

  // Check if input is in jersey number format (e.g., "99NYY" or "99-NYY")
  // First try to parse as number followed by team code
  int jersey_num = 0;
  char team_abbr[MAX_TEAM_NAME_LEN] = {0};
  char *dash = strchr(search_name, '-');

  if (dash != NULL) {
    // Format with hyphen: "99-NYY"
    *dash = '\0';
    jersey_num = atoi(search_name);
    strncpy(team_abbr, dash + 1, sizeof(team_abbr) - 1);
    team_abbr[sizeof(team_abbr) - 1] = '\0';
  } else {
    // Try format without hyphen: "99NYY"
    // Extract leading digits
    int i = 0;
    while (search_name[i] && isdigit((unsigned char)search_name[i])) {
      i++;
    }

    if (i > 0 && search_name[i] != '\0') {
      // We have digits followed by letters
      char jersey_str[MAX_NAME_LEN];
      strncpy(jersey_str, search_name, i);
      jersey_str[i] = '\0';
      jersey_num = atoi(jersey_str);
      strncpy(team_abbr, search_name + i, sizeof(team_abbr) - 1);
      team_abbr[sizeof(team_abbr) - 1] = '\0';
    }
  }

  // If we successfully parsed a jersey number and team, search for it
  if (jersey_num > 0 && strlen(team_abbr) > 0) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (player_roster[i].j_num == jersey_num) {
        // Check if team matches
        char player_team_upper[MAX_TEAM_NAME_LEN];
        strncpy(player_team_upper, player_roster[i].team,
                sizeof(player_team_upper) - 1);
        player_team_upper[sizeof(player_team_upper) - 1] = '\0';
        to_uppercase(player_team_upper);

        if (strcmp(player_team_upper, team_abbr) == 0) {
          return &player_roster[i];
        }
      }
    }
    return NULL;
  }

  // Traditional name-based search
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (strcmp(player_roster[i].name, search_name) == 0) {
      return &player_roster[i];
    }
  }
  return NULL;
}

// Initialize game state
void init_game(GameState *game) {
  memset(game, 0, sizeof(GameState));
  game->inning = 1;
  game->rand_seed1 = g_rand_seed1;
  game->rand_seed2 = g_rand_seed2;
  game->half_inning_runs = 0;
  game->half_inning_hits = 0;
  game->half_inning_errors = 0;
}

// Clear bases
void clear_bases(GameState *game) {
  game->bases[0] = game->bases[1] = game->bases[2] = 0;
}

// Print game header
void print_header(void) {
  printf("\n");
  printf("========================================\n");
  printf("  BBC BASEBALL SIMULATION (1961)\n");
  printf("  Burgeson Baseball Computer\n");
  printf("========================================\n\n");
}

// Print team lineup
void print_lineup(const char *team_name, Player *team[]) {
  printf("\n%s\n\n", team_name);
  printf("NAME       TEAM AVG BATS\n\n");

  for (int i = 0; i < TEAM_SIZE; i++) {
    Player *p = team[i];
    printf("%-10s %-2d %4d %-18s .%03d  %c\n", p->name, i + 1, p->year, p->team,
           p->batting_avg, p->hand);
  }
  printf("\n");
}

// Select visiting team
void select_visiting_team(GameState *game) {
  char input[MAX_NAME_LEN];
  int players_selected = 0;
  int used[MAX_PLAYERS] = {0};

  printf("ENTER YOUR LINEUP BELOW\n\n");
  printf("VISITORS\n\n");
  printf("NAME       TEAM AVG BATS\n\n");

  while (players_selected < TEAM_SIZE) {
    printf(">");
    if (fgets(input, sizeof(input), stdin) == NULL) {
      continue;
    }

    // Remove newline
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0) {
      continue;
    }

    // Trim whitespace
    char *start = input;
    while (isspace((unsigned char)*start))
      start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
      end--;
    *(end + 1) = '\0';

    Player *player = find_player(start);

    if (player == NULL) {
      printf("NON-VALID PLAYER. RETRY.\n");
      continue;
    }

    // Check if already selected
    int player_idx = (int)(player - player_roster);
    if (used[player_idx]) {
      printf("PLAYER ALREADY SELECTED. RETRY.\n");
      continue;
    }

    // Add to team
    game->visiting_team[players_selected] = player;
    used[player_idx] = 1;

    // Print player info
    printf("        %-2s %4d %-18s .%03d  %c\n",
           position_names[player->position / 10], player->year, player->team,
           player->batting_avg, player->hand);

    players_selected++;

    // Mix player info into random seeds
    g_rand_seed1 ^= (player->batting_avg * 31 + player_idx);
    g_rand_seed2 ^= (player->year * 37 + player->hand);
  }
}

// Select home team (computer picks randomly)
void select_home_team(GameState *game) {
  int used[MAX_PLAYERS] = {0};

  // Mark visiting team players as used
  for (int i = 0; i < TEAM_SIZE; i++) {
    int idx = (int)(game->visiting_team[i] - player_roster);
    used[idx] = 1;
  }

  // Build list of available players
  int available[MAX_PLAYERS];
  int available_count = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (!used[i]) {
      available[available_count++] = i;
    }
  }

  // Randomly select home team
  printf("\n\nHOME TEAM\n\n");
  printf("NAME       TEAM AVG BATS\n\n");

  for (int i = 0; i < TEAM_SIZE; i++) {
    int selection = random_number(0, available_count - 1);
    int player_idx = available[selection];

    game->home_team[i] = &player_roster[player_idx];

    // Print player info
    Player *p = game->home_team[i];
    printf("%-10s %-2s %4d %-18s .%03d  %c\n", p->name,
           position_names[p->position / 10], p->year, p->team, p->batting_avg,
           p->hand);

    // Remove from available list
    for (int j = selection; j < available_count - 1; j++) {
      available[j] = available[j + 1];
    }
    available_count--;
  }

  printf("\n\n");
}

// Determine play result based on batting average and randomness
PlayType determine_play_result(Player *batter, GameState *game) {
  int roll = random_number(1, 1000);
  int hit_threshold = batter->batting_avg + 50; // Bonus for simulation

  if (roll <= hit_threshold) {
    // Hit - determine type
    int hit_type = random_number(1, 100);

    if (hit_type <= 5) {
      return PLAY_HOME_RUN;
    } else if (hit_type <= 12) {
      return PLAY_TRIPLE;
    } else if (hit_type <= 30) {
      return PLAY_DOUBLE;
    } else {
      return PLAY_SINGLE;
    }
  } else {
    // Out - determine type
    int out_type = random_number(1, 100);

    if (out_type <= 3 && game->bases[0] && game->outs < 2) {
      return PLAY_DOUBLE_PLAY;
    } else if (out_type <= 8 && game->outs < 2 &&
               (game->bases[1] || game->bases[2])) {
      return PLAY_SACRIFICE_FLY;
    } else if (out_type <= 15) {
      return PLAY_STRIKEOUT_SWINGING;
    } else if (out_type <= 20) {
      return PLAY_STRIKEOUT_CALLED;
    } else if (out_type <= 25) {
      return PLAY_WALK;
    } else if (out_type <= 30) {
      return PLAY_ERROR;
    } else if (out_type <= 32 && game->bases[0]) {
      return PLAY_FIELDERS_CHOICE;
    } else if (out_type <= 33 && game->bases[0] && game->bases[1] &&
               game->outs == 0) {
      return PLAY_TRIPLE_PLAY;
    } else if (out_type <= 65) {
      return PLAY_GROUND_OUT;
    } else if (out_type <= 85) {
      return PLAY_FLY_OUT;
    } else {
      return PLAY_LINE_OUT;
    }
  }
}

// Print play result with descriptive text
void print_play_result(Player *batter, PlayType play, int runs_scored,
                       GameState *game) {
  printf("%s UP  ", batter->name);

  const char *location;
  int variant;

  switch (play) {
  case PLAY_SINGLE:
    variant = random_number(1, 10);
    location = field_locations[random_number(0, 9)];
    if (variant <= 2) {
      printf("SINGLE OVER %s", location);
    } else if (variant <= 3) {
      printf("INF. HIT TO %s", location);
    } else {
      printf("SINGLE TO %s", location);
    }
    break;

  case PLAY_DOUBLE:
    location = field_locations[random_number(0, 4)];
    variant = random_number(1, 10);
    if (variant <= 2) {
      printf("TEXAS LEAGER DOUBLE TO %s", location);
    } else if (variant <= 3) {
      printf("DOUBLE OVER %s", location);
    } else {
      printf("DOUBLE TO %s", location);
    }
    break;

  case PLAY_TRIPLE:
    location = field_locations[random_number(0, 4)];
    printf("TRIPLE TO %s", location);
    break;

  case PLAY_HOME_RUN:
    variant = random_number(1, 10);
    location = field_locations[random_number(0, 2)];
    if (variant <= 3) {
      printf("HOMER  TO %s", location);
    } else if (variant <= 4) {
      printf("BLAST OVER C F WALL");
    } else {
      printf("HOME RUN TO %s", location);
    }
    break;

  case PLAY_GROUND_OUT:
    location = field_locations[random_number(5, 9)];
    variant = random_number(1, 10);
    if (variant <= 2 && game->bases[0]) {
      printf("GROUNDER TO %s BATTER SAFE AT FIRST RUNNER OUT AT SECOND",
             location);
    } else if (variant <= 3 && game->bases[0] && game->bases[1]) {
      printf("GROUNDER TO %s BATTER SAFE AT FIRST RUNNER OUT IN RUNDOWN",
             location);
    } else if (variant <= 4 && game->bases[2] && game->outs < 2) {
      printf("GROUNDER TO %s BATTER SAFE AT FIRST RUNNER OUT AT HOME",
             location);
    } else {
      printf("GROUNDER TO %s", location);
    }
    break;

  case PLAY_FLY_OUT:
    location = field_locations[random_number(0, 4)];
    variant = random_number(1, 10);
    if (variant <= 2) {
      printf("LONG FLY TO %s", location);
    } else if (variant <= 3) {
      printf("SHORT FLY TO %s", location);
    } else if (variant <= 4) {
      printf("POP FLY TO %s", location);
    } else if (variant <= 5) {
      printf("FOUL OUT TO %s", location);
    } else {
      printf("FLY BALL TO %s", location);
    }
    break;

  case PLAY_LINE_OUT:
    location = field_locations[random_number(5, 9)];
    printf("LINE DRIVE TO %s", location);
    break;

  case PLAY_STRIKEOUT_SWINGING:
    printf("STRUCK OUT SWINGING");
    break;

  case PLAY_STRIKEOUT_CALLED:
    printf("STRUCK OUT CALLED");
    break;

  case PLAY_WALK:
    printf("BASE   ON BALLS");
    break;

  case PLAY_ERROR:
    location = field_locations[random_number(5, 9)];
    printf("ERROR ON %s FIELDER", location);
    break;

  case PLAY_DOUBLE_PLAY:
    location = field_locations[random_number(5, 9)];
    printf("GROUNDER TO %s DOUBLE PLAY", location);
    break;

  case PLAY_TRIPLE_PLAY:
    printf("LINE DRIVE TRIPLE PLAY");
    break;

  case PLAY_FIELDERS_CHOICE:
    printf("GROUNDER TO SHORT BATTER SAFE AT FIRST RUNNER OUT AT SECOND");
    break;

  case PLAY_SACRIFICE_FLY:
    location = field_locations[random_number(0, 4)];
    printf("LONG FLY TO %s", location);
    break;
  }

  if (runs_scored > 0) {
    if (runs_scored == 1) {
      printf(" RUNNER SCORES");
    } else if (runs_scored == 2) {
      printf(" TWO RUNS SCORE");
    } else if (runs_scored == 3) {
      printf(" 3 RUNS COME IN");
    } else {
      printf(" %d RUNS SCORE", runs_scored);
    }
  }

  printf("\n");
}

// Print base situation
void print_base_situation(GameState *game) {
  if (game->bases[0] && game->bases[1] && game->bases[2]) {
    printf(" BASES LOADED");
  } else if (game->bases[0] && game->bases[1]) {
    printf(" RUNNERS ON 1ST AND 2ND");
  } else if (game->bases[1] && game->bases[2]) {
    printf(" RUNNERS ON 2ND AND 3RD");
  } else if (game->bases[0] && game->bases[2]) {
    printf(" RUNNERS ON 1ST AND 3RD");
  } else if (game->bases[2]) {
    printf(" RUNNER ON 3RD");
  } else if (game->bases[1]) {
    printf(" RUNNER ON 2ND");
  } else if (game->bases[0]) {
    printf(" RUNNER ON 1ST");
  }
}

// Calculate runs scored when runners advance
int calculate_runs_scored(GameState *game, int bases_to_advance) {
  int runs = 0;

  // Check each base
  for (int i = 2; i >= 0; i--) {
    if (game->bases[i]) {
      int new_base = i + bases_to_advance;
      if (new_base >= 3) {
        runs++;
        game->bases[i] = 0;
      }
    }
  }

  return runs;
}

// Advance runners
void advance_runners(GameState *game, int bases_to_advance) {
  // Move runners backwards to avoid conflicts
  int new_bases[3] = {0, 0, 0};

  for (int i = 2; i >= 0; i--) {
    if (game->bases[i]) {
      int new_pos = i + bases_to_advance;
      if (new_pos < 3) {
        new_bases[new_pos] = 1;
      }
    }
  }

  memcpy(game->bases, new_bases, sizeof(new_bases));
}

// Execute a play
void execute_play(GameState *game, PlayType play, Player *batter) {
  int runs = 0;
  int *hits = game->is_bottom ? &game->home_hits : &game->visiting_hits;
  int *errors = game->is_bottom ? &game->home_errors : &game->visiting_errors;

  switch (play) {
  case PLAY_SINGLE:
    runs = calculate_runs_scored(game, 1);
    advance_runners(game, 1);
    game->bases[0] = 1;
    (*hits)++;
    game->half_inning_hits++;
    break;

  case PLAY_DOUBLE:
    runs = calculate_runs_scored(game, 2);
    advance_runners(game, 2);
    game->bases[1] = 1;
    (*hits)++;
    game->half_inning_hits++;
    break;

  case PLAY_TRIPLE:
    runs = calculate_runs_scored(game, 3);
    clear_bases(game);
    game->bases[2] = 1;
    (*hits)++;
    game->half_inning_hits++;
    break;

  case PLAY_HOME_RUN:
    runs = 1; // Batter scores
    for (int i = 0; i < 3; i++) {
      if (game->bases[i])
        runs++;
    }
    clear_bases(game);
    (*hits)++;
    game->half_inning_hits++;
    break;

  case PLAY_WALK:
    if (game->bases[0] && game->bases[1] && game->bases[2]) {
      runs = 1;
    } else if (game->bases[0] && game->bases[1]) {
      game->bases[2] = 1;
    } else if (game->bases[0]) {
      game->bases[1] = 1;
    }
    game->bases[0] = 1;
    break;

  case PLAY_ERROR:
    // Similar to single
    runs = calculate_runs_scored(game, 1);
    advance_runners(game, 1);
    game->bases[0] = 1;
    (*errors)++;
    game->half_inning_errors++;
    break;

  case PLAY_DOUBLE_PLAY:
    game->outs += 2;
    clear_bases(game);
    break;

  case PLAY_TRIPLE_PLAY:
    game->outs += 3;
    clear_bases(game);
    break;

  case PLAY_FIELDERS_CHOICE:
    game->outs++;
    advance_runners(game, 1);
    game->bases[0] = 1;
    break;

  case PLAY_SACRIFICE_FLY:
    game->outs++;
    if (game->bases[2]) {
      runs = 1;
      game->bases[2] = 0;
    }
    break;

  case PLAY_GROUND_OUT:
  case PLAY_FLY_OUT:
  case PLAY_LINE_OUT:
  case PLAY_STRIKEOUT_SWINGING:
  case PLAY_STRIKEOUT_CALLED:
    game->outs++;
    break;
  }

  // Add runs to score
  if (game->is_bottom) {
    game->home_score += runs;
  } else {
    game->visiting_score += runs;
  }
  game->half_inning_runs += runs;

  print_play_result(batter, play, runs, game);

  // Occasionally show base situation
  if (random_number(1, 3) == 1 &&
      (game->bases[0] || game->bases[1] || game->bases[2])) {
    print_base_situation(game);
    printf("\n");
  }
}

// Play at bat
void play_at_bat(GameState *game, Player *batter) {
  // Handle base stealing before the at-bat
  if (random_number(1, 12) == 1 && game->bases[0] && !game->bases[1] &&
      game->outs < 2) {
    printf("RUNNER STEALS SECOND\n");
    game->bases[1] = 1;
    game->bases[0] = 0;
  } else if (random_number(1, 20) == 1 && game->bases[0] && game->bases[1] &&
             !game->bases[2] && game->outs < 2) {
    printf("RUNNER STEALS SECOND\n");
    game->bases[1] = 1;
    game->bases[0] = 0;
  } else if (random_number(1, 25) == 1 && game->bases[0] && game->outs < 2) {
    printf("RUNNER OUT STEALING SECOND\n");
    game->bases[0] = 0;
    game->outs++;
    if (game->outs >= 3)
      return;
  }

  PlayType play = determine_play_result(batter, game);
  execute_play(game, play, batter);
}

// Print inning summary
void print_inning_summary(GameState *game) {
  printf("\n%d RUNS  %d HITS  %d ERRORS\n", game->half_inning_runs,
         game->half_inning_hits, game->half_inning_errors);
}

// Play game
void play_game(GameState *game) {
  while (game->inning <= MAX_INNINGS ||
         game->visiting_score == game->home_score) {
    // Top of inning (visitors bat)
    game->is_bottom = 0;
    game->outs = 0;
    clear_bases(game);
    game->half_inning_runs = 0;
    game->half_inning_hits = 0;
    game->half_inning_errors = 0;

    while (game->outs < 3) {
      Player *batter = game->visiting_team[game->current_batter];
      play_at_bat(game, batter);
      game->current_batter = (game->current_batter + 1) % TEAM_SIZE;
    }

    print_inning_summary(game);

    // Bottom of inning (home team bats)
    game->is_bottom = 1;
    game->outs = 0;
    clear_bases(game);
    game->half_inning_runs = 0;
    game->half_inning_hits = 0;
    game->half_inning_errors = 0;

    while (game->outs < 3) {
      Player *batter = game->home_team[game->current_batter];
      play_at_bat(game, batter);
      game->current_batter = (game->current_batter + 1) % TEAM_SIZE;
    }

    print_inning_summary(game);

    printf("\nEND OF INNING %d    SCORE %d %d\n\n", game->inning,
           game->visiting_score, game->home_score);

    game->inning++;

    // Check if game should end
    if (game->inning > MAX_INNINGS &&
        game->home_score != game->visiting_score) {
      break;
    }
  }
}

// Print final score
void print_final_score(GameState *game) {
  printf("\n\n\n");
  printf("GAME COMPLETED. TOTALS\n\n");
  printf("VISITORS        %02d  %02d  %02d\n", game->visiting_score,
         game->visiting_hits, game->visiting_errors);
  printf("HOMETEAM        %02d  %02d  %02d\n", game->home_score,
         game->home_hits, game->home_errors);
  printf("\n\n");
}

// Main function
int main(void) {
  char date_input[32];
  char time_input[32];

  console_setup();

  print_header();

  // Get date and time for random seed
  printf("TODAYS DATE IS >");
  if (fgets(date_input, sizeof(date_input), stdin) == NULL) {
    strncpy(date_input, "111", sizeof(date_input) - 1),
        date_input[sizeof(date_input) - 1] = '\0';
  }
  date_input[strcspn(date_input, "\n")] = 0;

  printf("\n\nTHE TIME IS >");
  if (fgets(time_input, sizeof(time_input), stdin) == NULL) {
    strncpy(time_input, "343", sizeof(time_input) - 1),
        time_input[sizeof(time_input) - 1] = '\0';
  }
  time_input[strcspn(time_input, "\n")] = 0;

  init_random(date_input, time_input);

  printf("\n\n");

  // Initialize and play game
  GameState game;
  init_game(&game);

  select_visiting_team(&game);
  select_home_team(&game);

  play_game(&game);
  print_final_score(&game);

  printf("\n\nPress Enter to exit...");
  getchar();

  return 0;
}