/*
 * Oregon Trail Simulation (1847)
 * Main game implementation
 * Original BASIC programming by Bill Heinemann - 1971
 */

#include "oregon.h"

// Platform-specific string copy
#ifdef UNIVAC
#define SAFE_STRCPY(dest, src, size) do { strncpy(dest, src, (size)-1); (dest)[(size)-1] = '\0'; } while(0)
#else
#define SAFE_STRCPY(dest, src, size) strcpy_s(dest, size, src)
#endif

// Event probability data (matches original BASIC DATA statement)
const int event_probabilities[15] = {
    6, 11, 13, 15, 17, 22, 32, 35, 37, 42, 44, 54, 64, 69, 95
};

// Date strings for each turn
static const char* date_strings[] = {
    "MARCH 29", "APRIL 12", "APRIL 26", "MAY 10", "MAY 24", "JUNE 7",
    "JUNE 21", "JULY 5", "JULY 19", "AUGUST 2", "AUGUST 16", "AUGUST 31",
    "SEPTEMBER 13", "SEPTEMBER 27", "OCTOBER 11", "OCTOBER 25",
    "NOVEMBER 8", "NOVEMBER 22", "DECEMBER 6", "DECEMBER 20"
};

// Shooting words for hunting mini-game
static const char* shooting_words[] = {"BANG", "BLAM", "POW", "WHAM"};

// Global random state
static unsigned int g_rand_seed = 0;

// Initialize random number generator
void init_random(void) {
#ifdef UNIVAC
    g_rand_seed = 0x12345678; // Fixed seed for UNIVAC compatibility
#else
    g_rand_seed = (unsigned int)time(NULL);
#endif
}

// Generate random integer in range [min, max]
int random_int(int min, int max) {
    g_rand_seed = (g_rand_seed * 1103515245 + 12345) & 0x7fffffff;
    return min + (g_rand_seed % (max - min + 1));
}

// Generate random double in range [0.0, 1.0)
double random_double(void) {
    g_rand_seed = (g_rand_seed * 1103515245 + 12345) & 0x7fffffff;
    return (double)g_rand_seed / (double)0x7fffffff;
}

// Main entry point
int main(void) {
#ifndef UNIVAC
    console_setup();
#endif
    
    GameState game;
    init_game(&game);
    
    printf("DO YOU NEED INSTRUCTIONS (YES/NO)? ");
    char response[MAX_INPUT_LEN];
    fgets(response, sizeof(response), stdin);
    to_uppercase(response);
    
    if (strstr(response, "YES") != NULL) {
        show_instructions();
    }
    
    setup_initial_purchases(&game);
    main_game_loop(&game);
    
    return 0;
}

// Initialize game state
void init_game(GameState* game) {
    init_random();
    
    memset(game, 0, sizeof(GameState));
    game->cash = AVAILABLE_MONEY;
    game->fort_available = -1; // Start with fort option available
    game->rand_seed = g_rand_seed;
}

// Show game instructions
void show_instructions(void) {
    printf("\n");
    printf("THIS PROGRAM SIMULATES A TRIP OVER THE OREGON TRAIL FROM\n");
    printf("INDEPENDENCE, MISSOURI TO OREGON CITY, OREGON IN 1847.\n");
    printf("YOUR FAMILY OF FIVE WILL COVER THE 2040 MILE OREGON TRAIL\n");
    printf("IN 5-6 MONTHS --- IF YOU MAKE IT ALIVE.\n");
    printf("\n");
    printf("YOU HAD SAVED $900 TO SPEND FOR THE TRIP, AND YOU'VE JUST\n");
    printf("   PAID $200 FOR A WAGON.\n");
    printf("YOU WILL NEED TO SPEND THE REST OF YOUR MONEY ON THE\n");
    printf("   FOLLOWING ITEMS:\n");
    printf("\n");
    printf("     OXEN - YOU CAN SPEND $200-$300 ON YOUR TEAM\n");
    printf("            THE MORE YOU SPEND, THE FASTER YOU'LL GO\n");
    printf("               BECAUSE YOU'LL HAVE BETTER ANIMALS\n");
    printf("\n");
    printf("     FOOD - THE MORE YOU HAVE, THE LESS CHANCE THERE\n");
    printf("               IS OF GETTING SICK\n");
    printf("\n");
    printf("     AMMUNITION - $1 BUYS A BELT OF 50 BULLETS\n");
    printf("            YOU WILL NEED BULLETS FOR ATTACKS BY ANIMALS\n");
    printf("               AND BANDITS, AND FOR HUNTING FOOD\n");
    printf("\n");
    printf("     CLOTHING - THIS IS ESPECIALLY IMPORTANT FOR THE COLD\n");
    printf("               WEATHER YOU WILL ENCOUNTER WHEN CROSSING\n");
    printf("               THE MOUNTAINS\n");
    printf("\n");
    printf("     MISCELLANEOUS SUPPLIES - THIS INCLUDES MEDICINE AND\n");
    printf("              OTHER THINGS YOU WILL NEED FOR SICKNESS\n");
    printf("              AND EMERGENCY REPAIRS\n");
    printf("\n");
    printf("YOU CAN SPEND ALL YOUR MONEY BEFORE YOU START YOUR TRIP -\n");
    printf("OR YOU CAN SAVE SOME OF YOUR CASH TO SPEND AT FORTS ALONG\n");
    printf("THE WAY WHEN YOU RUN LOW. HOWEVER, ITEMS COST MORE AT\n");
    printf("THE FORTS. YOU CAN ALSO GO HUNTING ALONG THE WAY TO GET\n");
    printf("MORE FOOD.\n");
    printf("WHENEVER YOU HAVE TO USE YOUR TRUSTY RIFLE ALONG THE WAY,\n");
    printf("YOU WILL BE TOLD TO TYPE IN THAT WORD (ONE THAT SOUNDS LIKE A\n");
    printf("GUN SHOT). THE FASTER YOU TYPE IN THAT WORD AND HIT THE\n");
    printf("\"RETURN\" KEY, THE BETTER LUCK YOU'LL HAVE WITH YOUR GUN.\n");
    printf("\n");
    printf("AT EACH TURN, ALL ITEMS ARE SHOWN IN DOLLAR AMOUNTS\n");
    printf("EXCEPT BULLETS\n");
    printf("WHEN ASKED TO ENTER MONEY AMOUNTS, DON'T USE A \"$\".\n");
    printf("\n");
    printf("GOOD LUCK!!!\n");
    printf("\n");
}

// Set up initial purchases
void setup_initial_purchases(GameState* game) {
    int total_spent;
    
    printf("HOW GOOD A SHOT ARE YOU WITH YOUR RIFLE?\n");
    printf("  (1) ACE MARKSMAN,  (2) GOOD SHOT,  (3) FAIR TO MIDDLIN'\n");
    printf("         (4) NEED MORE PRACTICE,  (5) SHAKY KNEES\n");
    printf("ENTER ONE OF THE ABOVE -- THE BETTER YOU CLAIM YOU ARE, THE\n");
    printf("FASTER YOU'LL HAVE TO BE WITH YOUR GUN TO BE SUCCESSFUL.\n");
    
    game->shooting_skill = get_user_choice("", 1, 5);
    
    do {
        printf("\n");
        
        // Oxen
        do {
            printf("HOW MUCH DO YOU WANT TO SPEND ON YOUR OXEN TEAM? ");
            scanf("%d", &game->oxen_cost);
            clear_input_buffer();
            
            if (game->oxen_cost < 200) {
                printf("NOT ENOUGH\n");
            } else if (game->oxen_cost > 300) {
                printf("TOO MUCH\n");
            }
        } while (game->oxen_cost < 200 || game->oxen_cost > 300);
        
        // Food
        do {
            printf("HOW MUCH DO YOU WANT TO SPEND ON FOOD? ");
            scanf("%d", &game->food);
            clear_input_buffer();
            
            if (game->food < 0) {
                printf("IMPOSSIBLE\n");
            }
        } while (game->food < 0);
        
        // Ammunition
        do {
            printf("HOW MUCH DO YOU WANT TO SPEND ON AMMUNITION? ");
            int ammo_cost;
            scanf("%d", &ammo_cost);
            clear_input_buffer();
            
            if (ammo_cost < 0) {
                printf("IMPOSSIBLE\n");
            } else {
                game->bullets = ammo_cost * 50; // $1 buys 50 bullets
                break;
            }
        } while (1);
        
        // Clothing
        do {
            printf("HOW MUCH DO YOU WANT TO SPEND ON CLOTHING? ");
            scanf("%d", &game->clothing);
            clear_input_buffer();
            
            if (game->clothing < 0) {
                printf("IMPOSSIBLE\n");
            }
        } while (game->clothing < 0);
        
        // Miscellaneous supplies
        do {
            printf("HOW MUCH DO YOU WANT TO SPEND ON MISCELLANEOUS SUPPLIES? ");
            scanf("%d", &game->misc_supplies);
            clear_input_buffer();
            
            if (game->misc_supplies < 0) {
                printf("IMPOSSIBLE\n");
            }
        } while (game->misc_supplies < 0);
        
        // Calculate remaining money
        total_spent = game->oxen_cost + game->food + (game->bullets / 50) + 
                     game->clothing + game->misc_supplies;
        game->cash = AVAILABLE_MONEY - total_spent;
        
        if (game->cash < 0) {
            printf("YOU OVERSPENT--YOU ONLY HAD $%d TO SPEND. BUY AGAIN\n", AVAILABLE_MONEY);
            // Reset for retry
            game->food = game->bullets = game->clothing = game->misc_supplies = 0;
        }
        
    } while (game->cash < 0);
    
    printf("AFTER ALL YOUR PURCHASES, YOU NOW HAVE $%d DOLLARS LEFT\n", game->cash);
    printf("\n");
    printf("MONDAY MARCH 29 1847\n");
    printf("\n");
}

// Main game loop
void main_game_loop(GameState* game) {
    while (game->miles_traveled < TOTAL_DISTANCE) {
        // Check if too much time has passed (winter death)
        if (game->turn_number >= 20) {
            printf("YOU HAVE BEEN ON THE TRAIL TOO LONG ------\n");
            printf("YOUR FAMILY DIES IN THE FIRST BLIZZARD OF WINTER\n");
            handle_death(game, DEATH_WINTER_BLIZZARD);
            return;
        }
        
        process_turn(game);
    }
    
    // Victory!
    check_victory_condition(game);
}

// Process a single turn
void process_turn(GameState* game) {
    game->turn_number++;
    game->miles_previous_turn = game->miles_traveled;
    
    // Print date
    printf("MONDAY ");
    print_current_date(game->turn_number);
    printf("1847\n\n");
    
    // Validate and fix negative resources
    validate_resources(game);
    
    // Pay doctor bill if needed
    pay_doctor_bill(game);
    
    // Display current status
    display_status(game);
    
    // Check for low food warning
    if (game->food < 13) {
        printf("YOU'D BETTER DO SOME HUNTING OR BUY FOOD SOON!!!!\n");
    }
    
    // Get player choice for this turn
    int choice;
    if (game->fort_available == -1) {
        printf("DO YOU WANT TO (1) STOP AT THE NEXT FORT, (2) HUNT, OR (3) CONTINUE\n");
        choice = get_user_choice("", 1, 3);
    } else {
        printf("DO YOU WANT TO (1) HUNT, OR (2) CONTINUE\n");
        choice = get_user_choice("", 1, 2);
        choice++; // Adjust for missing fort option
    }
    
    handle_turn_choice(game, choice);
    game->fort_available *= -1; // Toggle fort availability
}

// Display current game status
void display_status(GameState* game) {
    if (game->miles_traveled < 950) {
        printf("TOTAL MILEAGE IS %d\n", game->miles_traveled);
    } else {
        printf("TOTAL MILEAGE IS 950\n"); // Hide actual mileage in mountains
    }
    
    printf("FOOD\t\tBULLETS\t\tCLOTHING\tMISC. SUPP.\tCASH\n");
    printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", 
           game->food, game->bullets, game->clothing, game->misc_supplies, game->cash);
}

// Handle player's turn choice
void handle_turn_choice(GameState* game, int choice) {
    switch (choice) {
        case 1: // Stop at fort
            visit_fort(game);
            game->miles_traveled -= 45; // Time lost stopping at fort
            break;
            
        case 2: // Hunt
            if (game->bullets < 40) {
                printf("TOUGH---YOU NEED MORE BULLETS TO GO HUNTING\n");
                return;
            }
            go_hunting(game);
            game->miles_traveled -= 45; // Time lost hunting
            break;
            
        case 3: // Continue
            break;
    }
    
    // Continue with turn after choice
    travel_segment(game);
}

// Visit a fort for supplies
void visit_fort(GameState* game) {
    printf("ENTER WHAT YOU WISH TO SPEND ON THE FOLLOWING\n");
    
    // Food (2/3 efficiency due to higher fort prices)
    int amount = get_purchase_amount("FOOD", game->cash);
    game->food += (amount * 2) / 3;
    game->cash -= amount;
    
    // Ammunition (2/3 efficiency, $1 = 50 bullets normally)
    amount = get_purchase_amount("AMMUNITION", game->cash);
    game->bullets += ((amount * 2) / 3) * 50;
    game->cash -= amount;
    
    // Clothing (2/3 efficiency)
    amount = get_purchase_amount("CLOTHING", game->cash);
    game->clothing += (amount * 2) / 3;
    game->cash -= amount;
    
    // Miscellaneous supplies (2/3 efficiency)
    amount = get_purchase_amount("MISCELLANEOUS SUPPLIES", game->cash);
    game->misc_supplies += (amount * 2) / 3;
    game->cash -= amount;
}

// Get purchase amount with validation
int get_purchase_amount(const char* item_name, int max_money) {
    int amount;
    
    do {
        printf("%s? ", item_name);
        scanf("%d", &amount);
        clear_input_buffer();
        
        if (amount < 0) {
            amount = 0;
        } else if (amount > max_money) {
            printf("YOU DON'T HAVE THAT MUCH--KEEP YOUR SPENDING DOWN\n");
            printf("YOU MISS YOUR CHANCE TO SPEND ON THAT ITEM\n");
            amount = 0;
        }
    } while (amount > max_money);
    
    return amount;
}

// Go hunting mini-game
void go_hunting(GameState* game) {
    if (game->bullets < 40) {
        printf("TOUGH---YOU NEED MORE BULLETS TO GO HUNTING\n");
        return;
    }
    
    int shooting_result = shooting_minigame(game->shooting_skill);
    
    if (shooting_result <= 1) {
        printf("RIGHT BETWEEN THE EYES---YOU GOT A BIG ONE!!!!\n");
        printf("FULL BELLIES TONIGHT!\n");
        game->food += 52 + random_int(0, 6);
        game->bullets -= 10 + random_int(0, 4);
    } else if (random_double() * 100 < 13 * shooting_result) {
        printf("YOU MISSED---AND YOUR DINNER GOT AWAY.....\n");
        game->bullets -= 10 + 3 * shooting_result;
    } else {
        printf("NICE SHOT--RIGHT ON TARGET--GOOD EATIN' TONIGHT!!\n");
        game->food += 48 - 2 * shooting_result;
        game->bullets -= 10 + 3 * shooting_result;
    }
}

// Shooting mini-game implementation
int shooting_minigame(int skill_level) {
    char word_buffer[10];
    char input[MAX_INPUT_LEN];
    int word_index = random_int(0, 3);
    
    strcpy(word_buffer, shooting_words[word_index]);
    printf("TYPE %s\n", word_buffer);
    
    // Simple timing - in a real implementation you'd measure actual response time
    fgets(input, sizeof(input), stdin);
    
    // Remove newline
    input[strcspn(input, "\n")] = '\0';
    to_uppercase(input);
    
    // Check if word matches - adjust result based on skill level
    if (strcmp(input, word_buffer) == 0) {
        return skill_level > 3 ? skill_level - 2 : 1; // Better skill = better result
    } else {
        return 9; // Miss due to wrong word
    }
}

// Travel segment and events
void travel_segment(GameState* game) {
    // Check for starvation before travel
    if (game->food < 13) {
        handle_death(game, DEATH_STARVATION);
        return;
    }
    
    // Handle eating
    check_eating_and_health(game);
    
    // Calculate travel distance based on oxen and random factors
    int base_travel = 200 + (game->oxen_cost - 220) / 3 + random_int(1, 10) * 10;
    game->miles_traveled += base_travel;
    
    // Check for rider attacks
    check_for_riders(game);
    
    // Process random events
    process_random_events(game);
    
    // Handle mountain travel if in mountain region
    if (game->miles_traveled > 950) {
        mountain_travel(game);
    }
}

// Check for rider encounters
void check_for_riders(GameState* game) {
    double attack_chance = pow((game->miles_traveled / 100.0 - 4), 2) + 72;
    attack_chance = attack_chance / (attack_chance - 60) - 1;
    
    if (random_double() * 10 > attack_chance) {
        return; // No riders
    }
    
    printf("RIDERS AHEAD. THEY ");
    
    int hostile = (random_double() < 0.8) ? 0 : 1;
    if (random_double() > 0.2) {
        hostile = 1 - hostile; // Flip hostility randomly
    }
    
    if (!hostile) {
        printf("DON'T ");
    }
    printf("LOOK HOSTILE\n");
    
    printf("TACTICS\n");
    printf("(1) RUN  (2) ATTACK  (3) CONTINUE  (4) CIRCLE WAGONS\n");
    
    int tactic = get_user_choice("", 1, 4);
    handle_rider_encounter(game, hostile);
    
    // Handle tactic results based on hostility
    if (!hostile) {
        // Friendly riders
        switch (tactic) {
            case 1: // Run
                game->miles_traveled += 15;
                game->oxen_cost -= 10;
                break;
            case 2: // Attack
                game->miles_traveled -= 5;
                game->bullets -= 100;
                break;
            case 3: // Continue
                break;
            case 4: // Circle wagons
                game->miles_traveled -= 20;
                break;
        }
        printf("RIDERS WERE FRIENDLY, BUT CHECK FOR POSSIBLE LOSSES\n");
    } else {
        // Hostile riders
        switch (tactic) {
            case 1: // Run
                game->miles_traveled += 20;
                game->misc_supplies -= 15;
                game->bullets -= 150;
                game->oxen_cost -= 40;
                break;
            case 2: // Attack
                {
                    int shooting_result = shooting_minigame(game->shooting_skill);
                    game->bullets -= shooting_result * 40 + 80;
                    
                    if (shooting_result <= 1) {
                        printf("NICE SHOOTING---YOU DROVE THEM OFF\n");
                    } else if (shooting_result <= 4) {
                        printf("KINDA SLOW WITH YOUR COLT .45\n");
                    } else {
                        printf("LOUSY SHOT---YOU GOT KNIFED\n");
                        game->game_flags |= FLAG_INJURY;
                        printf("YOU HAVE TO SEE OL' DOC BLANCHARD\n");
                    }
                }
                break;
            case 3: // Continue
                if (random_double() > 0.8) {
                    printf("THEY DID NOT ATTACK\n");
                    return;
                }
                game->bullets -= 150;
                game->misc_supplies -= 15;
                break;
            case 4: // Circle wagons
                {
                    int shooting_result = shooting_minigame(game->shooting_skill);
                    game->bullets -= shooting_result * 30 + 80;
                    game->miles_traveled -= 25;
                }
                break;
        }
        printf("RIDERS WERE HOSTILE--CHECK FOR LOSSES\n");
    }
    
    // Check if ran out of bullets
    if (game->bullets < 0) {
        printf("YOU RAN OUT OF BULLETS AND GOT MASSACRED BY THE RIDERS\n");
        handle_death(game, DEATH_MASSACRE);
    }
}

// Handle rider encounter results
void handle_rider_encounter(GameState* game, int hostile) {
    // This function processes the encounter results
    // Most of the logic is handled in check_for_riders
    (void)game;    // Suppress unused parameter warning
    (void)hostile; // Suppress unused parameter warning
}

// Process random events
void process_random_events(GameState* game) {
    int random_val = (int)(random_double() * 100);
    int event_index = 0;
    
    // Find which event occurs based on probability table
    for (int i = 0; i < 15; i++) {
        if (random_val <= event_probabilities[i]) {
            event_index = i;
            break;
        }
    }
    
    if (event_index == 15) {
        event_index = 15; // Helpful Indians (last event)
    }
    
    handle_event(game, (EventType)event_index);
}

// Handle specific events
void handle_event(GameState* game, EventType event) {
    switch (event) {
        case EVENT_WAGON_BREAKDOWN:
            printf("WAGON BREAKS DOWN--LOSE TIME AND SUPPLIES FIXING IT\n");
            game->miles_traveled -= 15 + random_int(1, 5) * 5;
            game->misc_supplies -= 8;
            break;
            
        case EVENT_OX_INJURY:
            printf("OX INJURES LEG---SLOWS YOU DOWN REST OF TRIP\n");
            game->miles_traveled -= 25;
            game->oxen_cost -= 20;
            break;
            
        case EVENT_DAUGHTER_BREAKS_ARM:
            printf("BAD LUCK---YOUR DAUGHTER BROKE HER ARM\n");
            printf("YOU HAD TO STOP AND USE SUPPLIES TO MAKE A SLING\n");
            game->miles_traveled -= 5 + random_int(1, 4) * 4;
            game->misc_supplies -= 2 + random_int(1, 3) * 3;
            break;
            
        case EVENT_OX_WANDERS_OFF:
            printf("OX WANDERS OFF---SPEND TIME LOOKING FOR IT\n");
            game->miles_traveled -= 17;
            break;
            
        case EVENT_SON_GETS_LOST:
            printf("YOUR SON GETS LOST---SPEND HALF THE DAY LOOKING FOR HIM\n");
            game->miles_traveled -= 10;
            break;
            
        case EVENT_UNSAFE_WATER:
            printf("UNSAFE WATER--LOSE TIME LOOKING FOR CLEAN SPRING\n");
            game->miles_traveled -= random_int(1, 10) * 10 + 2;
            break;
            
        case EVENT_HEAVY_RAINS:
            if (game->miles_traveled <= 950) {
                printf("HEAVY RAINS---TIME AND SUPPLIES LOST\n");
                game->food -= 10;
                game->bullets -= 500;
                game->misc_supplies -= 15;
                game->miles_traveled -= random_int(1, 10) * 10 + 5;
            }
            break;
            
        case EVENT_BANDITS_ATTACK:
            printf("BANDITS ATTACK\n");
            {
                int shooting_result = shooting_minigame(game->shooting_skill);
                game->bullets -= 20 * shooting_result;
                
                if (game->bullets < 0) {
                    printf("YOU RAN OUT OF BULLETS---THEY GET LOTS OF CASH\n");
                    game->cash /= 3;
                } else if (shooting_result <= 1) {
                    printf("QUICKEST DRAW OUTSIDE OF DODGE CITY!!!\n");
                    printf("YOU GOT 'EM!\n");
                } else {
                    printf("YOU GOT SHOT IN THE LEG AND THEY TOOK ONE OF YOUR OXEN\n");
                    game->game_flags |= FLAG_INJURY;
                    printf("BETTER HAVE A DOC LOOK AT YOUR WOUND\n");
                    game->misc_supplies -= 5;
                    game->oxen_cost -= 20;
                }
            }
            break;
            
        case EVENT_FIRE_IN_WAGON:
            printf("THERE WAS A FIRE IN YOUR WAGON--FOOD AND SUPPLIES DAMAGE!\n");
            game->food -= 40;
            game->bullets -= 400;
            game->misc_supplies -= random_int(1, 8) * 8 + 3;
            game->miles_traveled -= 15;
            break;
            
        case EVENT_LOSE_WAY_IN_FOG:
            printf("LOSE YOUR WAY IN HEAVY FOG---TIME IS LOST\n");
            game->miles_traveled -= 10 + random_int(1, 5) * 5;
            break;
            
        case EVENT_POISONOUS_SNAKE:
            printf("YOU KILLED A POISONOUS SNAKE AFTER IT BIT YOU\n");
            game->bullets -= 10;
            game->misc_supplies -= 5;
            if (game->misc_supplies < 0) {
                printf("YOU DIE OF SNAKEBITE SINCE YOU HAVE NO MEDICINE\n");
                handle_death(game, DEATH_SNAKEBITE);
            }
            break;
            
        case EVENT_WAGON_SWAMPED_FORDING:
            printf("WAGON GETS SWAMPED FORDING RIVER--LOSE FOOD AND CLOTHES\n");
            game->food -= 30;
            game->clothing -= 20;
            game->miles_traveled -= 20 + random_int(1, 20) * 20;
            break;
            
        case EVENT_WILD_ANIMALS_ATTACK:
            printf("WILD ANIMALS ATTACK!\n");
            {
                int shooting_result = shooting_minigame(game->shooting_skill);
                
                if (game->bullets < 40) {
                    printf("YOU WERE TOO LOW ON BULLETS--\n");
                    printf("THE WOLVES OVERPOWERED YOU\n");
                    game->game_flags |= FLAG_INJURY;
                    handle_illness(game);
                } else {
                    if (shooting_result <= 2) {
                        printf("NICE SHOOTIN' PARDNER---THEY DIDN'T GET MUCH\n");
                    } else {
                        printf("SLOW ON THE DRAW---THEY GOT AT YOUR FOOD AND CLOTHES\n");
                    }
                    
                    game->bullets -= 20 * shooting_result;
                    game->clothing -= shooting_result * 4;
                    game->food -= shooting_result * 8;
                }
            }
            break;
            
        case EVENT_COLD_WEATHER:
            if (game->miles_traveled > 950) {
                printf("COLD WEATHER---BRRRRRRR!--YOU ");
                if (game->clothing > 22 + random_int(1, 4) * 4) {
                    printf("HAVE ENOUGH CLOTHING TO KEEP YOU WARM\n");
                } else {
                    printf("DON'T HAVE ENOUGH CLOTHING TO KEEP YOU WARM\n");
                    handle_illness(game);
                }
            }
            break;
            
        case EVENT_HAIL_STORM:
            printf("HAIL STORM---SUPPLIES DAMAGED\n");
            game->miles_traveled -= 5 + random_int(1, 10) * 10;
            game->bullets -= 200;
            game->misc_supplies -= 4 + random_int(1, 3) * 3;
            break;
            
        case EVENT_HELPFUL_INDIANS:
            printf("HELPFUL INDIANS SHOW YOU WHERE TO FIND MORE FOOD\n");
            game->food += 14;
            break;
    }
}

// Handle mountain travel
void mountain_travel(GameState* game) {
    if (random_double() * 10 <= 9 - (pow(game->miles_traveled / 100.0 - 15, 2) + 72) / 
        (pow(game->miles_traveled / 100.0 - 15, 2) + 12)) {
        return;
    }
    
    printf("RUGGED MOUNTAINS\n");
    
    if (random_double() <= 0.1) {
        printf("YOU GOT LOST---LOSE VALUABLE TIME TRYING TO FIND TRAIL!\n");
        game->miles_traveled -= 60;
    } else if (random_double() <= 0.11) {
        printf("WAGON DAMAGED!---LOSE TIME AND SUPPLIES\n");
        game->misc_supplies -= 5;
        game->bullets -= 200;
        game->miles_traveled -= 20 + random_int(1, 30) * 30;
    } else {
        printf("THE GOING GETS SLOW\n");
        game->miles_traveled -= 45 + (int)(random_double() / 0.02);
    }
    
    // Check for mountain pass events
    check_mountain_events(game);
}

// Check for mountain-specific events
void check_mountain_events(GameState* game) {
    // South Pass
    if (!(game->game_flags & FLAG_SOUTH_PASS) && random_double() < 0.8) {
        printf("YOU MADE IT SAFELY THROUGH SOUTH PASS--NO SNOW\n");
        game->game_flags |= FLAG_SOUTH_PASS;
        return;
    }
    
    // Blue Mountains  
    if (game->miles_traveled >= 1700 && !(game->game_flags & FLAG_BLUE_MOUNTAINS) && 
        random_double() < 0.7) {
        game->game_flags |= FLAG_BLUE_MOUNTAINS;
        return;
    }
    
    // Blizzard
    printf("BLIZZARD IN MOUNTAIN PASS--TIME AND SUPPLIES LOST\n");
    game->game_flags |= FLAG_BLIZZARD;
    game->food -= 25;
    game->misc_supplies -= 10;
    game->bullets -= 300;
    game->miles_traveled -= 30 + random_int(1, 40) * 40;
    
    if (game->clothing < 18 + random_int(1, 2) * 2) {
        handle_illness(game);
    }
}

// Check eating and health
void check_eating_and_health(GameState* game) {
    printf("DO YOU WANT TO EAT (1) POORLY (2) MODERATELY\n");
    printf("OR (3) WELL? ");
    
    game->eating_level = get_user_choice("", 1, 3);
    
    int food_consumed = 8 + 5 * game->eating_level;
    
    if (game->food < food_consumed) {
        printf("YOU CAN'T EAT THAT WELL\n");
        game->eating_level = 1; // Force poor eating
        food_consumed = 13;
    }
    
    game->food -= food_consumed;
    
    // Check for illness based on eating level
    if (game->eating_level != 1) {
        // Check illness probability
        if (random_double() * 100 < 10 + 35 * (game->eating_level - 1)) {
            return; // No illness
        }
    }
    
    if (random_double() * 100 < 100 - (40 / pow(4, game->eating_level - 1))) {
        handle_illness(game);
    }
}

// Handle illness
void handle_illness(GameState* game) {
    double illness_roll = random_double() * 100;
    
    if (illness_roll < 10 + 35 * (game->eating_level - 1)) {
        printf("MILD ILLNESS---MEDICINE USED\n");
        game->miles_traveled -= 5;
        game->misc_supplies -= 2;
    } else if (illness_roll < 100 - (40 / pow(4, game->eating_level - 1))) {
        printf("BAD ILLNESS---MEDICINE USED\n");
        game->miles_traveled -= 5;
        game->misc_supplies -= 2;
    } else {
        printf("SERIOUS ILLNESS---\n");
        printf("YOU MUST STOP FOR MEDICAL ATTENTION\n");
        game->misc_supplies -= 10;
        game->game_flags |= FLAG_ILLNESS;
    }
    
    if (game->misc_supplies < 0) {
        printf("YOU RAN OUT OF MEDICAL SUPPLIES\n");
        handle_death(game, DEATH_DISEASE);
    }
}

// Validate and fix negative resources
void validate_resources(GameState* game) {
    if (game->food < 0) game->food = 0;
    if (game->bullets < 0) game->bullets = 0;
    if (game->clothing < 0) game->clothing = 0;
    if (game->misc_supplies < 0) game->misc_supplies = 0;
}

// Pay doctor bill if injured or ill
void pay_doctor_bill(GameState* game) {
    if (game->game_flags & (FLAG_ILLNESS | FLAG_INJURY)) {
        game->cash -= 20;
        if (game->cash < 0) {
            game->cash = 0;
            printf("YOU CAN'T AFFORD A DOCTOR\n");
            handle_death(game, DEATH_DISEASE);
            return;
        }
        printf("DOCTOR'S BILL IS $20\n");
        game->game_flags &= ~(FLAG_ILLNESS | FLAG_INJURY); // Clear flags
    }
}

// Check victory condition
void check_victory_condition(GameState* game) {
    if (game->miles_traveled >= TOTAL_DISTANCE) {
        show_victory_scene(game);
    }
}

// Handle death scenarios
void handle_death(GameState* game, DeathCause cause) {
    (void)game; // Suppress unused parameter warning
    
    show_death_scene(cause);
    
    printf("\n");
    printf("DUE TO YOUR UNFORTUNATE SITUATION, THERE ARE A FEW\n");
    printf("FORMALITIES WE MUST GO THROUGH\n");
    printf("\n");
    
    if (get_yes_no_input("WOULD YOU LIKE A MINISTER? ")) {
        // Minister selected
    }
    
    if (get_yes_no_input("WOULD YOU LIKE A FANCY FUNERAL? ")) {
        // Fancy funeral selected  
    }
    
    if (get_yes_no_input("WOULD YOU LIKE US TO INFORM YOUR NEXT OF KIN? ")) {
        printf("THAT WILL BE $50 FOR THE TELEGRAPH CHARGE.\n");
    } else {
        printf("BUT YOUR AUNT SADIE IN ST. LOUIS IS REALLY WORRIED ABOUT YOU\n");
    }
    
    printf("\n");
    printf("WE THANK YOU FOR THIS INFORMATION AND WE ARE SORRY YOU\n");
    printf("DIDN'T MAKE IT TO THE GREAT TERRITORY OF OREGON\n");
    printf("BETTER LUCK NEXT TIME\n");
    printf("\n");
    printf("\t\t\tSINCERELY\n");
    printf("\n");
    printf("\t\tTHE OREGON CITY CHAMBER OF COMMERCE\n");
    
    exit(0);
}

// Show death message based on cause
void show_death_scene(DeathCause cause) {
    switch (cause) {
        case DEATH_STARVATION:
            printf("YOU RAN OUT OF FOOD AND STARVED TO DEATH\n");
            break;
        case DEATH_EXHAUSTION:
            printf("YOU DIED OF EXHAUSTION\n");
            break;
        case DEATH_DISEASE:
            printf("YOU DIED OF PNEUMONIA\n");
            break;
        case DEATH_INJURIES:
            printf("YOU DIED OF INJURIES\n");
            break;
        case DEATH_WINTER_BLIZZARD:
            printf("YOUR FAMILY DIES IN THE FIRST BLIZZARD OF WINTER\n");
            break;
        case DEATH_SNAKEBITE:
            printf("YOU DIE OF SNAKEBITE SINCE YOU HAVE NO MEDICINE\n");
            break;
        case DEATH_MASSACRE:
            printf("YOU RAN OUT OF BULLETS AND GOT MASSACRED BY THE RIDERS\n");
            break;
    }
}

// Show victory scene
void show_victory_scene(GameState* game) {
    printf("\n");
    printf("YOU FINALLY ARRIVED AT OREGON CITY\n");
    printf("AFTER 2040 LONG MILES---HOORAY!!!!!\n");
    printf("A REAL PIONEER!\n");
    printf("\n");
    
    calculate_final_date(game);
    
    printf("\n");
    printf("FOOD\t\tBULLETS\t\tCLOTHING\tMISC. SUPP.\tCASH\n");
    validate_resources(game);
    printf("%d\t\t%d\t\t%d\t\t%d\t\t%d\n", 
           game->food, game->bullets, game->clothing, game->misc_supplies, game->cash);
    
    printf("\n");
    printf("\t\tPRESIDENT JAMES K. POLK SENDS YOU HIS\n");
    printf("\t\t\tHEARTIEST CONGRATULATIONS\n");
    printf("\n");
    printf("\t\tAND WISHES YOU A PROSPEROUS LIFE AHEAD\n");
    printf("\n");
    printf("\t\t\tAT YOUR NEW HOME\n");
    
    exit(0);
}

// Calculate and display final arrival date
void calculate_final_date(GameState* game) {
    double fraction = (double)(TOTAL_DISTANCE - game->miles_previous_turn) / 
                     (double)(game->miles_traveled - game->miles_previous_turn);
    
    // Calculate final supplies (not used for display but kept for completeness)
    // int final_food = game->food + (int)((1 - fraction) * (8 + 5 * game->eating_level));
    int days_into_turn = (int)(fraction * 14);
    int total_days = game->turn_number * 14 + days_into_turn;
    
    // Calculate day of week (starting from Monday = 0)
    int day_of_week = (total_days + 1) % 7;
    const char* day_names[] = {"MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", 
                              "FRIDAY", "SATURDAY", "SUNDAY"};
    
    printf("%s ", day_names[day_of_week]);
    
    // Calculate date
    if (total_days <= 124) {
        printf("JULY %d 1847\n", total_days - 93);
    } else if (total_days <= 155) {
        printf("AUGUST %d 1847\n", total_days - 124);
    } else if (total_days <= 185) {
        printf("SEPTEMBER %d 1847\n", total_days - 155);
    } else if (total_days <= 216) {
        printf("OCTOBER %d 1847\n", total_days - 185);
    } else if (total_days <= 246) {
        printf("NOVEMBER %d 1847\n", total_days - 216);
    } else {
        printf("DECEMBER %d 1847\n", total_days - 246);
    }
}

// Print current date for turn
void print_current_date(int turn_number) {
    if (turn_number > 0 && turn_number <= 20) {
        printf("%s ", date_strings[turn_number - 1]);
    }
}

// Get user choice with validation
int get_user_choice(const char* prompt, int min_choice, int max_choice) {
    int choice;
    
    do {
        if (strlen(prompt) > 0) {
            printf("%s", prompt);
        }
        scanf("%d", &choice);
        clear_input_buffer();
        
        if (choice < min_choice || choice > max_choice) {
            choice = max_choice; // Default to last valid option
        }
    } while (choice < min_choice || choice > max_choice);
    
    return choice;
}

// Get yes/no input
int get_yes_no_input(const char* prompt) {
    char response[MAX_INPUT_LEN];
    
    printf("%s", prompt);
    fgets(response, sizeof(response), stdin);
    to_uppercase(response);
    
    return (strstr(response, "YES") != NULL) ? 1 : 0;
}

// Clear input buffer
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Wait for keypress
void wait_for_keypress(void) {
    printf("Press Enter to continue...\n");
    clear_input_buffer();
}

// Print separator line
void print_separator(void) {
    printf("========================================\n");
}

// Convert string to uppercase
void to_uppercase(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

// Console setup for Windows
#ifndef UNIVAC
void console_setup(void) {
    // Enable ANSI escape codes on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    // Set console title
    SetConsoleTitle("Oregon Trail 1847");
}
#endif
