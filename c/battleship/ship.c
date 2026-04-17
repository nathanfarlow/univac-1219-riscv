/*
 * ship.c - Ship implementation for Battleship game
 * Cross-platform compatible
 */

#include "battleship.h"

/* Initialize a ship */
void init_ship(Ship* s, const char* name, int length) {
    SAFE_STRCPY(s->name, name, MAX_NAME_LENGTH);
    s->length = length;
    s->position_count = 0;
}

/* Store ship placement coordinates */
void store_ship_placement(Ship* s, char roF, char roS, int coF, int coS) {
    int i;
    char coord[MAX_COORD_LENGTH];

    s->position_count = 0;

    if (roF == roS) {
        /* Horizontal placement */
        for (i = coF; i <= coS; i++) {
            SAFE_SPRINTF(coord, MAX_COORD_LENGTH, "%c%d", roF, i);
            SAFE_STRCPY(s->positions[s->position_count], coord, MAX_COORD_LENGTH);
            s->position_count++;
        }
    } else {
        /* Vertical placement */
        for (i = roF; i <= roS; i++) {
            SAFE_SPRINTF(coord, MAX_COORD_LENGTH, "%c%d", (char)i, coF);
            SAFE_STRCPY(s->positions[s->position_count], coord, MAX_COORD_LENGTH);
            s->position_count++;
        }
    }
}

/* Check if coordinate is part of this ship */
int is_part_of_ship(Ship* s, char row, int col) {
    int i;
    char coord[MAX_COORD_LENGTH];
    SAFE_SPRINTF(coord, MAX_COORD_LENGTH, "%c%d", row, col);

    for (i = 0; i < s->position_count; i++) {
        if (strcmp(s->positions[i], coord) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Remove a ship part when hit */
void remove_ship_part(Ship* s, char row, int col) {
    int i, j;
    char coord[MAX_COORD_LENGTH];
    SAFE_SPRINTF(coord, MAX_COORD_LENGTH, "%c%d", row, col);

    for (i = 0; i < s->position_count; i++) {
        if (strcmp(s->positions[i], coord) == 0) {
            /* Shift remaining positions */
            for (j = i; j < s->position_count - 1; j++) {
                SAFE_STRCPY(s->positions[j], s->positions[j + 1], MAX_COORD_LENGTH);
            }
            s->position_count--;
            break;
        }
    }
}

/* Check if ship is sunken */
int is_ship_sunken(Ship* s) {
    return s->position_count == 0;
}
