/*
 * player.c - Player implementation for Battleship game
 * Cross-platform compatible
 */

#include "battleship.h"

/* Initialize a player */
void init_player(Player* p, const char* name) {
    SAFE_STRCPY(p->name, name, MAX_NAME_LENGTH);
    init_battlefield(&p->arena);
    p->ship_count = NO_OF_SHIPS;

    /* Initialize all ships */
    init_ship(&p->ships[0], "AIRCRAFT CARRIER", 5);
    init_ship(&p->ships[1], "BATTLESHIP", 4);
    init_ship(&p->ships[2], "CRUISER", 3);
    init_ship(&p->ships[3], "SUBMARINE", 3);
    init_ship(&p->ships[4], "DESTROYER", 2);
}

/* Check if all ships are sunken */
int is_navy_sunken(Player* p) {
    return p->ship_count == 0;
}

/* Manage ship hit - print message and update ship status */
void manage_ship_hit(Player* p, char row, int col) {
    int i, j;
    int found = 0;

    place_piece(&p->arena, row, col, HIT);

    for (i = 0; i < NO_OF_SHIPS && !found; i++) {
        if (i >= p->ship_count) continue;

        if (is_part_of_ship(&p->ships[i], row, col)) {
            remove_ship_part(&p->ships[i], row, col);

            if (is_ship_sunken(&p->ships[i])) {
                printf("YOU SANK A SHIP!\n");
                /* Remove ship from array by shifting */
                for (j = i; j < p->ship_count - 1; j++) {
                    p->ships[j] = p->ships[j + 1];
                }
                p->ship_count--;
            } else {
                printf("YOU HIT A SHIP!\n");
            }
            found = 1;
        }
    }
}
