/*
 * SQLSteel - Ultra-Lightweight In-Memory Database for UNIVAC 1219
 * Main Program Module
 * 
 * Memory Budget: 40kB total
 * Features: Full SQL parser with SELECT, INSERT, DELETE, UPDATE
 * Data: 118 elements from the Periodic Table
 * 
 * Designed for 1960s UNIVAC - All data stored in runtime memory
 * No disk I/O - everything created and destroyed in memory
 */

#include "sqlsteel.h"

#include "sqlsteel.h"

/* Display statistics */
void cmd_stats(void) {
    printf("\n=== DATABASE STATISTICS ===\n");
    printf("TOTAL ACTIVE RECORDS: %d\n", get_active_count());
    printf("TOTAL DELETED SLOTS: %d\n", MAX_RECORDS - get_active_count());
    printf("MEMORY USAGE: ~%d BYTES\n", (int)(sizeof(g_database) + sizeof(g_record_count)));
}

/* Display help menu */
void cmd_help(void) {
    printf("\n=== SQLSTEEL - SQL COMMAND REFERENCE ===\n");
    printf("\nSUPPORTED SQL COMMANDS:\n");
    printf("\n1. SELECT:\n");
    printf("   SELECT * FROM ELEMENTS;\n");
    printf("   SELECT * FROM ELEMENTS WHERE ATOMIC_WEIGHT > 100;\n");
    printf("   SELECT DISTINCT * FROM ELEMENTS WHERE SERIES = 'NOBLE GAS';\n");
    printf("   SELECT * FROM ELEMENTS ORDER BY ATOMIC_WEIGHT DESC LIMIT 10;\n");
    printf("   SELECT * FROM ELEMENTS WHERE ELEMENT_NAME LIKE 'CARBON%%';\n");
    printf("\n2. INSERT:\n");
    printf("   INSERT INTO ELEMENTS VALUES ('NAME', 'SYMBOL', WEIGHT, 'SERIES');\n");
    printf("   Example: INSERT INTO ELEMENTS VALUES ('UNOBTAINIUM', 'Un', 999.9, 'SYNTHETIC');\n");
    printf("\n3. UPDATE:\n");
    printf("   UPDATE ELEMENTS SET ATOMIC_WEIGHT = 12.011 WHERE ELEMENT_NUMBER = 6;\n");
    printf("   UPDATE ELEMENTS SET SERIES = 'UPDATED' WHERE SYMBOL = 'H';\n");
    printf("\n4. DELETE:\n");
    printf("   DELETE FROM ELEMENTS WHERE ELEMENT_NUMBER = 119;\n");
    printf("\nQUERY MODIFIERS:\n");
    printf("   DISTINCT      - Remove duplicate rows\n");
    printf("   ORDER BY      - Sort results (ASC/DESC)\n");
    printf("   LIMIT n       - Return only n rows\n");
    printf("   OFFSET n      - Skip first n rows\n");
    printf("   FETCH FIRST n ROWS ONLY - SQL standard limit\n");
    printf("\nOPERATORS:\n");
    printf("   Comparison: =, >, <, >=, <=, <>, !=\n");
    printf("   LIKE         - Pattern matching (%% = any chars, _ = single char)\n");
    printf("   IN           - Match any value in list\n");
    printf("   BETWEEN      - Range: ATOMIC_WEIGHT BETWEEN 10 AND 50\n");
    printf("\nLOGICAL OPERATORS:\n");
    printf("   AND          - Both conditions must be true\n");
    printf("   OR           - At least one condition must be true\n");
    printf("   NOT          - Negate condition\n");
    printf("\nEXAMPLES:\n");
    printf("   SELECT * FROM ELEMENTS WHERE ATOMIC_WEIGHT BETWEEN 50 AND 100;\n");
    printf("   SELECT * FROM ELEMENTS WHERE SERIES IN ('ALKALI METAL', 'NOBLE GAS');\n");
    printf("   SELECT * FROM ELEMENTS WHERE SERIES = 'TRANSITION METAL' LIMIT 10;\n");
    printf("   SELECT DISTINCT * FROM ELEMENTS WHERE ELEMENT_NAME LIKE 'C%%';\n");
    printf("\nOTHER COMMANDS:\n");
    printf("   STATS - Show database statistics\n");
    printf("   HELP  - Show this menu\n");
    printf("   EXIT  - Terminate program\n");
    printf("\nFIELDS: ELEMENT_NUMBER (or NUM), ELEMENT_NAME (or NAME), SYMBOL, ATOMIC_WEIGHT (or WEIGHT), SERIES\n");
    printf("\nMEMORY: NO DISK STORAGE - ALL DATA IN RAM\n");
}

/* Main program */
int main(void) {
    char sql_buffer[MAX_SQL_LEN];
    
    printf("\n");
    printf("===================================================\n");
    printf("  SQLSTEEL - IN-MEMORY DATABASE SYSTEM\n");
    printf("===================================================\n");
    printf("\n");
    
    init_database();
    load_demo_data();
    
    printf("\nSYSTEM READY. TYPE 'HELP' FOR COMMAND REFERENCE.\n");
    printf("TYPE SQL COMMANDS OR 'EXIT' TO QUIT.\n");
    
    while (1) {
        printf("\nSQL> ");
        
        if (fgets(sql_buffer, MAX_SQL_LEN, stdin) == NULL) {
            break;
        }
        
        /* Remove newline */
        sql_buffer[strcspn(sql_buffer, "\n")] = '\0';
        
        /* Skip empty lines */
        if (strlen(sql_buffer) == 0) continue;
        
        /* Convert to uppercase for comparison */
        char cmd_check[MAX_SQL_LEN];
        PLATFORM_STRCPY(cmd_check, MAX_SQL_LEN, sql_buffer);
        to_upper(cmd_check);
        
        /* Check for special commands */
        if (strcmp(cmd_check, "EXIT") == 0 || strcmp(cmd_check, "QUIT") == 0) {
            printf("\nTERMINATING SQLSTEEL...\n");
            break;
        }
        else if (strcmp(cmd_check, "HELP") == 0) {
            cmd_help();
        }
        else if (strcmp(cmd_check, "STATS") == 0) {
            cmd_stats();
        }
        else {
            /* Execute SQL command */
            execute_sql(sql_buffer);
        }
    }
    
    return 0;
}
