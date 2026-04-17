/*
 * Display Module
 * Handles formatted output of records
 */

#include "sqlsteel.h"

/* Print table header */
void print_header(void) {
    print_separator();
    printf("# NUM # ELEMENT NAME  # SYM # WEIGHT  # SERIES               #\n");
    print_separator();
}

/* Print separator line */
void print_separator(void) {
    printf("#-----#---------------#-----#---------#-----------------------#\n");
}

/* Print single record (table format) */
void print_record(ElementRecord *rec) {
    if (rec->active) {
        printf("# %-3d # %-13s # %-3s # %-7.3f # %-21s #\n",
               rec->element_number, rec->element_name, rec->symbol,
               rec->atomic_weight, rec->series);
    }
}

/* Print full record details */
void print_full_record(ElementRecord *rec) {
    printf("\n=== ELEMENT NUMBER: %d ===\n", rec->element_number);
    printf("ELEMENT NAME:       %s\n", rec->element_name);
    printf("SYMBOL:             %s\n", rec->symbol);
    printf("ATOMIC WEIGHT:      %.3f\n", rec->atomic_weight);
    printf("SERIES:             %s\n", rec->series);
    printf("STATUS:             %s\n", rec->active ? "ACTIVE" : "DELETED");
    printf("============================\n");
}
