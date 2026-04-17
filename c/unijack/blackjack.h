/*
 * UNIJACK - Text-based Blackjack Game
 * Modern C implementation for Windows Console
 * Converted from Python implementation
 * 
 * Compiles with MSVC and MinGW
 */

#ifndef BLACKJACK_H
#define BLACKJACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Platform-specific includes
#ifndef UNIVAC
#include <windows.h>
#endif

// Platform-specific string copy
#ifdef UNIVAC
#define SAFE_STRCPY(dest, src, size) do { strncpy(dest, src, (size)-1); (dest)[(size)-1] = '\0'; } while(0)
#else
#define SAFE_STRCPY(dest, src, size) strcpy_s(dest, size, src)
#endif

// Constants
#define MAX_STRING_LEN 128
#define MAX_CARDS_IN_DECK 52
#define MAX_DECKS 8
#define MAX_CARDS_IN_SHOE (MAX_CARDS_IN_DECK * MAX_DECKS)
#define MAX_CARDS_IN_HAND 21
#define MAX_PLAYERS 7
#define MAX_NAME_LEN 64

#define TARGET_SCORE 21
#define MINIMUM_DEALER_SCORE 17

// Card constants
#define NUM_SUITS 4
#define NUM_RANKS 13

// Outcome constants
#define BUST 0
#define LOOSE 1
#define PUSH 2
#define WIN 3
#define BLACKJACK 4

// Card structure
typedef struct {
    int suit;       // 0=Spade, 1=Heart, 2=Diamond, 3=Club
    int rank;       // 0=Ace, 1=2, ..., 9=10, 10=Jack, 11=Queen, 12=King
    int visible;    // 1=visible, 0=hidden
} Card;

// Hand structure
typedef struct {
    Card cards[MAX_CARDS_IN_HAND];
    int card_count;
    int wager;
} Hand;

// Player structure
typedef struct {
    char name[MAX_NAME_LEN];
    int chip_count;
    Hand hand;
} Player;

// Dealer structure
typedef struct {
    Hand hand;
} Dealer;

// Shoe structure
typedef struct {
    Card cards[MAX_CARDS_IN_SHOE];
    int total_cards;
    int current_index;
    int auto_shuffling;
} Shoe;

// Ruleset structure
typedef struct {
    int maximum_player_count;
    int deck_count_in_shoe;
    int auto_shuffling_shoe;
    int minimum_wager;
    int dealer_receives_hole_card;
    int dealer_reveals_blackjack_hand;
    double blackjack_payout_ratio;
} Ruleset;

// Table structure
typedef struct {
    Shoe shoe;
    Dealer dealer;
    Player players[MAX_PLAYERS];
    int player_count;
    int active_player_indices[MAX_PLAYERS];
    int active_player_count;
} Table;

// Game state structure
typedef struct {
    Ruleset ruleset;
    int running;
} Game;

// BSS-initialized global state
#ifdef UNIVAC
// For UNIVAC, we ensure BSS initialization
extern Card g_deck_template[MAX_CARDS_IN_DECK];
extern int g_bss_initialized;
#endif

// Function declarations - Card operations
void init_card(Card* card, int suit, int rank);
void get_card_name(const Card* card, char* buffer, size_t buffer_size);
void get_card_values(const Card* card, int* values, int* value_count);

// Function declarations - Deck operations
void create_deck(Card* deck);

// Function declarations - Shoe operations
void init_shoe(Shoe* shoe, int deck_count, int auto_shuffling);
void shuffle_shoe(Shoe* shoe);
void reload_shoe(Shoe* shoe);
Card* draw_card(Shoe* shoe, int visible);

// Function declarations - Hand operations
void init_hand(Hand* hand);
void add_card_to_hand(Hand* hand, Card* card);
void reveal_all_cards(Hand* hand);
int get_hand_score(const Hand* hand);

// Function declarations - Score operations
int score_from_hand(const Hand* hand);
void compare_hands(const Hand* hand1, const Hand* hand2, int* outcome1, int* outcome2);

// Function declarations - Player operations
void init_player(Player* player, const char* name, int chip_count);
int bet_chips(Player* player, int chip_count);
void earn_chips(Player* player, int chip_count);
void drop_player_hand(Player* player);

// Function declarations - Dealer operations
void init_dealer(Dealer* dealer);
void drop_dealer_hand(Dealer* dealer);

// Function declarations - Table operations
void init_table(Table* table, const Ruleset* ruleset);

// Function declarations - Game operations
void init_game(Game* game, const Ruleset* ruleset);
void run_game(Game* game, Table* table);
int play_new_round(Game* game, Table* table);
int collect_wagers(Game* game, Table* table);
void deal_initial_cards(Game* game, Table* table);
void interact_with_player(Game* game, Table* table, int player_idx);
void interact_with_dealer(Game* game, Table* table);
void pay_gains(Game* game, Table* table);
void cleanup_table(Table* table);

// Function declarations - UI operations
void print_colored(const char* msg, const char* color);
void display_player(const Player* player);
void display_dealer(const Dealer* dealer);
int ask_integer(const char* msg, int default_value, int min_value, int max_value);
char ask_choice(const char* msg, const char* choices, char default_choice);

// Function declarations - Ruleset operations
void init_basic_ruleset(Ruleset* ruleset);
void init_european_ruleset(Ruleset* ruleset);
void init_american_ruleset(Ruleset* ruleset);

// Utility functions
void to_upper(char* str);
int safe_min(int a, int b);
int safe_max(int a, int b);

// Platform-specific functions
#ifndef UNIVAC
void console_setup(void);
#endif

#endif // BLACKJACK_H
