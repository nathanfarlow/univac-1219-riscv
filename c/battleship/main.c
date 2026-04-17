/*
 * main.c - Battleship Game Main Program
 * Cross-platform: Windows (MSVC/MinGW) and UNIVAC 1219 (GCC)
 *
 * Implements Battleship game with Intermediate Adversary AI
 * Uses XorShift RNG seeded with time(0) for UNIVAC compatibility
 */

#include "battleship.h"

int main(void) {
    Player human;
    Player ai_player;
    IntermediateAI ai_engine;
    unsigned int rng_state;
    char input[100];
    char shot[MAX_COORD_LENGTH];
    char shot_row;
    int shot_col;
    int i;
    int did_p1_win = 0;

    printf("\n========================================\n");
    printf("   BATTLESHIP - INTERMEDIATE AI\n");
    printf("   PLATFORM: %s\n", PLATFORM_NAME);
    printf("========================================\n\n");

    /* Initialize random number generator */
    init_random(&rng_state);

    /* Menu 1: Action Menu */
    printf("WHAT WOULD YOU LIKE TO DO?\n");
    printf("\t[S]TART\n\tE[X]IT\n");
    get_input(input, sizeof(input));

    if (input[0] == 'x' || input[0] == 'X') {
        printf("EXITING GAME. GOODBYE!\n");
        return 0;
    }

    /* Get human player name */
    printf("\nENTER YOUR NAME: ");
    get_input(input, sizeof(input));
    init_player(&human, input);

    /* Initialize AI player */
    init_player(&ai_player, "INTERMEDIATE AI");
    init_intermediate_ai(&ai_engine);

    printf("\n========================================\n");
    printf("   GAME SETUP\n");
    printf("========================================\n\n");

    /* Human places ships */
    printf("PLAYER 1, PLACE YOUR SHIPS ON THE GAME FIELD\n");
    print_battlefield(&human.arena, 0);

    for (i = 0; i < NO_OF_SHIPS; i++) {
        char roF, roS;
        int coF, coS;
        int placement_res;

        printf("\nPLACE %s (LENGTH %d)\n", human.ships[i].name, human.ships[i].length);
        printf("ENTER FIRST COORDINATE (E.G., A1): ");
        get_input(input, sizeof(input));
        roF = input[0];
        sscanf(input + 1, "%d", &coF);

        printf("ENTER SECOND COORDINATE (E.G., A5): ");
        get_input(input, sizeof(input));
        roS = input[0];
        sscanf(input + 1, "%d", &coS);

        /* Normalize coordinates */
        normalize_coordinates(&roF, &roS, &coF, &coS);

        /* Validate placement */
        placement_res = is_correct_coordinates(&human.arena, roF, roS, coF, coS, &human.ships[i]);

        if (placement_res != VALID_COORD) {
            printf("INVALID PLACEMENT! TRY AGAIN.\n");
            i--;
            continue;
        }

        /* Place ship on battlefield */
        if (roF == roS) {
            int j;
            for (j = coF; j <= coS; j++) {
                place_piece(&human.arena, roF, j, SHIP_PIECE);
            }
        } else {
            char j;
            for (j = roF; j <= roS; j++) {
                place_piece(&human.arena, j, coF, SHIP_PIECE);
            }
        }

        store_ship_placement(&human.ships[i], roF, roS, coF, coS);
        print_battlefield(&human.arena, 0);
    }

    prompt_enter_key();

    /* AI places ships */
    printf("\nKINDLY WAIT WHILE THE MACHINE PLACES ITS SHIPS\n");
    for (i = 0; i < NO_OF_SHIPS; i++) {
        ai_place_ship(&ai_player, i, &rng_state);
    }
    printf("\nTHE MACHINE HAS COMPLETED PLACING ITS SHIPS!\n\n");
    print_battlefield(&ai_player.arena, 0);

    prompt_enter_key();

    /* Wartime - Main game loop */
    printf("THE GAME STARTS!\n\n");

    while (1) {
        /* Display both battlefields */
        printf("ENEMY BATTLEFIELD:\n");
        print_battlefield(&ai_player.arena, 1);
        print_divider();
        printf("YOUR BATTLEFIELD:\n");
        print_battlefield(&human.arena, 0);

        /* Human fires */
        printf("ENTER COORDINATES TO FIRE AT (E.G., B5): ");
        get_input(input, sizeof(input));
        SAFE_STRCPY(shot, input, MAX_COORD_LENGTH);
        shot_row = shot[0];
        sscanf(shot + 1, "%d", &shot_col);

        /* Process human shot */
        if (is_hit(&ai_player.arena, shot_row, shot_col)) {
            ai_manage_ship_hit(&ai_player, &ai_engine, shot_row, shot_col);
        } else if (is_miss(&ai_player.arena, shot_row, shot_col)) {
            place_piece(&ai_player.arena, shot_row, shot_col, MISS);
            printf("YOU MISSED! TRY AGAIN NEXT TURN\n");
        } else {
            printf("ALREADY FIRED AT THIS LOCATION!\n");
        }

        /* Check if human won */
        if (is_navy_sunken(&ai_player)) {
            did_p1_win = 1;
            break;
        }

        /* AI fires */
        printf("\nPLEASE WAIT WHILE THE ENGINE MAKES ITS MOVE\n");
        ai_fire_salvo(&ai_engine, shot, &rng_state);
        printf("THE ENGINE FIRED AT %s\n", shot);
        shot_row = shot[0];
        sscanf(shot + 1, "%d", &shot_col);

        /* Process AI shot */
        if (is_hit(&human.arena, shot_row, shot_col)) {
            manage_ship_hit(&human, shot_row, shot_col);
        } else if (is_miss(&human.arena, shot_row, shot_col)) {
            place_piece(&human.arena, shot_row, shot_col, MISS);
            printf("THE ENGINE FIRED AT %s AND MISSED.\n", shot);
        }

        /* Check if AI won */
        if (is_navy_sunken(&human)) {
            break;
        }

        printf("\n");
    }

    /* Game end */
    printf("\n========================================\n");
    printf("   GAME OVER\n");
    printf("========================================\n\n");

    if (did_p1_win) {
        printf("CONGRATULATIONS %s, YOU HAVE WON THIS GAME OF BATTLESHIP!\n", human.name);
    } else {
        printf("THE INTERMEDIATE AI ENGINE WON THIS GAME OF BATTLESHIP!\n");
    }

    return 0;
}
