/*
 * ai_engine.c - Intermediate Adversary AI Engine for Battleship
 * Cross-platform compatible
 * Implements Hunt & Target algorithm
 */

#include "battleship.h"

/* Initialize the Intermediate AI engine */
void init_intermediate_ai(IntermediateAI* ai) {
    ai->target_count = 0;
    ai->hunt_count = 0;
    ai->targets_fired_count = 0;
    ai->is_targeting = 0;
    ai->previous_shot[0] = '\0';

    create_targets(ai);
}

/* Create target and hunt lists */
void create_targets(IntermediateAI* ai) {
    int i;

    /* TARGETS: All squares (0-99) */
    ai->target_count = 0;
    for (i = 0; i < 100; i++) {
        ai->targets[ai->target_count++] = i;
    }

    /* HUNTING: All even parity squares (checkerboard pattern) */
    ai->hunt_count = 0;
    for (i = 1; i < 100; i += 2) {
        if ((i / 10) % 2 == 0) {
            ai->hunts[ai->hunt_count++] = i;
        } else {
            ai->hunts[ai->hunt_count++] = i - 1;
        }
    }
}

/* Encode string coordinates to integer (A1 = 0, J10 = 99) */
int encode_coord(const char* coord) {
    char row = coord[0];
    int column;

    sscanf(coord + 1, "%d", &column);

    return (row - 'A') * 10 + column - 1;
}

/* Decode integer to string coordinates */
void decode_coord(int encoded, char* result) {
    char row = (char)(encoded / 10 + 'A');
    int column = encoded % 10 + 1;
    SAFE_SPRINTF(result, MAX_COORD_LENGTH, "%c%d", row, column);
}

/* Remove value from integer array */
static void remove_from_array(int* array, int* count, int value) {
    int i, j;
    for (i = 0; i < *count; i++) {
        if (array[i] == value) {
            for (j = i; j < *count - 1; j++) {
                array[j] = array[j + 1];
            }
            (*count)--;
            return;
        }
    }
}

/* Check if value exists in array */
static int contains_value(int* array, int count, int value) {
    int i;
    for (i = 0; i < count; i++) {
        if (array[i] == value) {
            return 1;
        }
    }
    return 0;
}

/* Hunt mode - fire at checkerboard pattern squares */
void hunt_squares(IntermediateAI* ai, char* result, unsigned int* rng_state) {
    int rand_index, rand_hunt_square;

    /* Clear targets fired stack */
    ai->targets_fired_count = 0;

    /* Pick from hunt list if available */
    if (ai->hunt_count > 0) {
        rand_index = random_range(rng_state, 0, ai->hunt_count - 1);
        rand_hunt_square = ai->hunts[rand_index];
        remove_from_array(ai->hunts, &ai->hunt_count, rand_hunt_square);
    } else if (ai->target_count > 0) {
        /* Fallback to target list */
        rand_index = random_range(rng_state, 0, ai->target_count - 1);
        rand_hunt_square = ai->targets[rand_index];
    } else {
        /* No squares left - shouldn't happen in normal game */
        SAFE_STRCPY(result, "A1", MAX_COORD_LENGTH);
        return;
    }

    /* Decode to battlefield coordinates */
    decode_coord(rand_hunt_square, result);

    /* Remove from targets list */
    remove_from_array(ai->targets, &ai->target_count, rand_hunt_square);
}

/* Target mode - fire at squares adjacent to hit */
void target_ship(IntermediateAI* ai, const char* previous_shot, int is_hit, char* result) {
    int starting_target = encode_coord(previous_shot);
    int north, south, east, west;
    int coordinate_to_fire;

    /* Calculate positions around the target */
    north = starting_target - 10;
    south = starting_target + 10;
    east = starting_target + 1;
    west = starting_target - 1;

    /* Add valid adjacent squares to firing stack */
    if (contains_value(ai->targets, ai->target_count, north) &&
        !contains_value(ai->targets_fired, ai->targets_fired_count, north) &&
        north >= 0) {
        ai->targets_fired[ai->targets_fired_count++] = north;
    }
    if (contains_value(ai->targets, ai->target_count, south) &&
        !contains_value(ai->targets_fired, ai->targets_fired_count, south) &&
        south < 100) {
        ai->targets_fired[ai->targets_fired_count++] = south;
    }
    if (contains_value(ai->targets, ai->target_count, east) &&
        !contains_value(ai->targets_fired, ai->targets_fired_count, east) &&
        (starting_target % 10 != 9)) {
        ai->targets_fired[ai->targets_fired_count++] = east;
    }
    if (contains_value(ai->targets, ai->target_count, west) &&
        !contains_value(ai->targets_fired, ai->targets_fired_count, west) &&
        (starting_target % 10 != 0)) {
        ai->targets_fired[ai->targets_fired_count++] = west;
    }

    /* If no valid targets, resume hunt mode */
    if (ai->targets_fired_count == 0) {
        hunt_squares(ai, result, NULL);
        return;
    }

    /* Pop from stack (take last element) */
    coordinate_to_fire = ai->targets_fired[--ai->targets_fired_count];

    /* Remove from hunt list if present */
    remove_from_array(ai->hunts, &ai->hunt_count, coordinate_to_fire);
    remove_from_array(ai->targets, &ai->target_count, coordinate_to_fire);

    decode_coord(coordinate_to_fire, result);
}

/* AI fires a salvo */
void ai_fire_salvo(IntermediateAI* ai, char* result, unsigned int* rng_state) {
    if (ai->is_targeting) {
        target_ship(ai, ai->previous_shot, 1, result);
    } else {
        hunt_squares(ai, result, rng_state);
    }

    SAFE_STRCPY(ai->previous_shot, result, MAX_COORD_LENGTH);
}

/* AI manages when its ship is hit */
void ai_manage_ship_hit(Player* p, IntermediateAI* ai, char row, int col) {
    int i, j;
    int found = 0;

    place_piece(&p->arena, row, col, HIT);

    for (i = 0; i < NO_OF_SHIPS && !found; i++) {
        if (i >= p->ship_count) continue;

        if (is_part_of_ship(&p->ships[i], row, col)) {
            remove_ship_part(&p->ships[i], row, col);

            if (is_ship_sunken(&p->ships[i])) {
                printf("YOU SANK A SHIP!\n");
                /* Remove ship from array */
                for (j = i; j < p->ship_count - 1; j++) {
                    p->ships[j] = p->ships[j + 1];
                }
                p->ship_count--;
                ai->is_targeting = 0;  /* Stop targeting when ship is sunk */
            } else {
                printf("YOU HIT A SHIP!\n");
                ai->is_targeting = 1;  /* Start targeting mode */
            }
            found = 1;
        }
    }
}

/* AI places a ship on the battlefield */
void ai_place_ship(Player* p, int ship_index, unsigned int* rng_state) {
    char row_start;
    int col_start;
    int horizontal_or_vertical, ne_or_sw;
    int placement_res;
    int ship_len = p->ships[ship_index].length;
    int i;
    char row_iter;

    while (1) {
        /* Generate random starting position */
        row_start = random_row(rng_state);
        col_start = random_col(rng_state);

        /* Decide horizontal or vertical */
        horizontal_or_vertical = random_range(rng_state, 0, 1);
        ne_or_sw = random_range(rng_state, 0, 1);

        if (horizontal_or_vertical == 0) {
            /* Try horizontal placement */
            if (ne_or_sw == 0 && col_start - ship_len + 1 >= 1) {
                /* Left direction */
                placement_res = is_correct_coordinates(&p->arena, row_start, row_start,
                    col_start - ship_len + 1, col_start, &p->ships[ship_index]);
                if (placement_res == VALID_COORD) {
                    for (i = col_start - ship_len + 1; i <= col_start; i++) {
                        place_piece(&p->arena, row_start, i, SHIP_PIECE);
                    }
                    store_ship_placement(&p->ships[ship_index], row_start, row_start,
                        col_start - ship_len + 1, col_start);
                    return;
                }
            } else if (ne_or_sw == 1 && col_start + ship_len - 1 <= 10) {
                /* Right direction */
                placement_res = is_correct_coordinates(&p->arena, row_start, row_start,
                    col_start, col_start + ship_len - 1, &p->ships[ship_index]);
                if (placement_res == VALID_COORD) {
                    for (i = col_start; i <= col_start + ship_len - 1; i++) {
                        place_piece(&p->arena, row_start, i, SHIP_PIECE);
                    }
                    store_ship_placement(&p->ships[ship_index], row_start, row_start,
                        col_start, col_start + ship_len - 1);
                    return;
                }
            }
        } else {
            /* Try vertical placement */
            if (ne_or_sw == 0 && row_start - ship_len + 1 >= 'A') {
                /* Up direction */
                placement_res = is_correct_coordinates(&p->arena,
                    (char)(row_start - ship_len + 1), row_start, col_start, col_start,
                    &p->ships[ship_index]);
                if (placement_res == VALID_COORD) {
                    for (row_iter = (char)(row_start - ship_len + 1); row_iter <= row_start; row_iter++) {
                        place_piece(&p->arena, row_iter, col_start, SHIP_PIECE);
                    }
                    store_ship_placement(&p->ships[ship_index],
                        (char)(row_start - ship_len + 1), row_start, col_start, col_start);
                    return;
                }
            } else if (ne_or_sw == 1 && row_start + ship_len - 1 <= 'J') {
                /* Down direction */
                placement_res = is_correct_coordinates(&p->arena, row_start,
                    (char)(row_start + ship_len - 1), col_start, col_start,
                    &p->ships[ship_index]);
                if (placement_res == VALID_COORD) {
                    for (row_iter = row_start; row_iter <= row_start + ship_len - 1; row_iter++) {
                        place_piece(&p->arena, row_iter, col_start, SHIP_PIECE);
                    }
                    store_ship_placement(&p->ships[ship_index], row_start,
                        (char)(row_start + ship_len - 1), col_start, col_start);
                    return;
                }
            }
        }
        /* If we didn't return, try again with new random position */
    }
}
