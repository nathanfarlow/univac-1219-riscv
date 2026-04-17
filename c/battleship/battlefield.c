/*
 * battlefield.c - Battlefield implementation for Battleship game
 * Cross-platform compatible
 */

#include "battleship.h"

/* Initialize battlefield with water */
void init_battlefield(Battlefield* bf) {
    int i, j;
    for (i = 0; i < BOARD_SIZE; i++) {
        for (j = 0; j < BOARD_SIZE; j++) {
            bf->board[i][j] = WATER;
        }
    }
}

/* Print battlefield - cloaked during wartime, exposed during setup */
void print_battlefield(Battlefield* bf, int is_wartime) {
    int i, j;
    char row_label;

    printf("\n  ");
    for (i = 1; i <= BOARD_SIZE; i++) {
        printf("%d ", i);
    }

    for (i = 0; i < BOARD_SIZE; i++) {
        row_label = 'A' + i;
        printf("\n%c ", row_label);
        for (j = 0; j < BOARD_SIZE; j++) {
            if (is_wartime && bf->board[i][j] == SHIP_PIECE) {
                printf("%c ", WATER);
            } else {
                printf("%c ", bf->board[i][j]);
            }
        }
    }
    printf("\n\n");
}

/* Check if coordinate hits a ship */
int is_hit(Battlefield* bf, char row, int col) {
    return get_piece(bf, row, col) == SHIP_PIECE;
}

/* Check if coordinate is a miss */
int is_miss(Battlefield* bf, char row, int col) {
    return get_piece(bf, row, col) == WATER;
}

/* Place a piece on the battlefield */
void place_piece(Battlefield* bf, char row, int col, char piece) {
    if (row >= 'A' && row <= 'J' && col >= 1 && col <= 10) {
        bf->board[row - 'A'][col - 1] = piece;
    }
}

/* Get piece at coordinate */
char get_piece(Battlefield* bf, char row, int col) {
    if (row >= 'A' && row <= 'J' && col >= 1 && col <= 10) {
        return bf->board[row - 'A'][col - 1];
    }
    return WATER;
}

/* Check if coordinates are valid for ship placement */
int is_correct_coordinates(Battlefield* bf, char roF, char roS, int coF, int coS, Ship* s) {
    /* Check for coordinates outside the board */
    if (roF > 'J' || roF < 'A' || roS > 'J' || roS < 'A') {
        return OUT_OF_BOARD;
    }
    if (coF > 10 || coF < 1 || coS > 10 || coS < 1) {
        return OUT_OF_BOARD;
    }

    /* Validate ship placement if ship is provided */
    if (s != NULL) {
        /* Check for coordinates not corresponding to straight lines */
        if (roF != roS && coF != coS) {
            return MISALIGN;
        }

        /* Check length matches ship */
        if (roF == roS) {
            if (abs(coF - coS) + 1 != s->length) {
                return WRONG_LENGTH;
            }
        } else {
            if (abs(roF - roS) + 1 != s->length) {
                return WRONG_LENGTH;
            }
        }

        /* Check if crossing or touching other ships */
        if (is_crossing(bf, roF, roS, coF, coS)) {
            return CROSSING;
        }
        if (is_touching(bf, roF, roS, coF, coS)) {
            return TOUCHING;
        }
    }

    return VALID_COORD;
}

/* Check if ship crosses another ship */
int is_crossing(Battlefield* bf, char roF, char roS, int coF, int coS) {
    int i, j;
    for (i = roF - 'A'; i <= roS - 'A'; i++) {
        for (j = coF - 1; j <= coS - 1; j++) {
            if (bf->board[i][j] == SHIP_PIECE) {
                return 1;
            }
        }
    }
    return 0;
}

/* Check if ship is touching another ship */
int is_touching(Battlefield* bf, char roF, char roS, int coF, int coS) {
    int i;
    int start_row = roF - 'A';
    int end_row = roS - 'A';
    int start_col = coF - 1;
    int end_col = coS - 1;

    /* Check horizontal adjacency */
    for (i = start_row; i <= end_row; i++) {
        /* Check left */
        if (start_col > 0 && bf->board[i][start_col - 1] == SHIP_PIECE) {
            return 1;
        }
        /* Check right */
        if (end_col < BOARD_SIZE - 1 && bf->board[i][end_col + 1] == SHIP_PIECE) {
            return 1;
        }
    }

    return 0;
}
