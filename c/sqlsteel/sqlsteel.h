/*
 * SQLSteel Header File
 * Ultra-Lightweight In-Memory Database for UNIVAC 1219
 */

#ifndef SQLSTEEL_H
#define SQLSTEEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

/* Platform-specific configuration */
#ifdef UNIVAC
    /* UNIVAC-specific macros for portability */
    /* Safe string copy - ensures null termination and avoids truncation warnings */
    static inline void platform_strcpy_safe(char *dest, size_t dest_size, const char *src) {
        if (dest_size > 0) {
            size_t i;
            for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
                dest[i] = src[i];
            }
            dest[i] = '\0';
        }
    }
    
    #define PLATFORM_STRCPY(dest, dest_size, src) platform_strcpy_safe(dest, dest_size, src)
    #define CLEAR_SCREEN() printf("\n\n\n")  /* Simple clear for UNIVAC */
    
    /* Fixed-width types for UNIVAC compatibility */
    typedef int32_t sql_int_t;
    typedef uint16_t sql_uint16_t;
#else
    /* Windows-specific includes */
    /* Note: Currently not using windows.h - fully portable */
    
    static inline void platform_strcpy_safe(char *dest, size_t dest_size, const char *src) {
        if (dest_size > 0) {
            size_t i;
            for (i = 0; i < dest_size - 1 && src[i] != '\0'; i++) {
                dest[i] = src[i];
            }
            dest[i] = '\0';
        }
    }
    
    #define PLATFORM_STRCPY(dest, dest_size, src) platform_strcpy_safe(dest, dest_size, src)
    #define CLEAR_SCREEN() printf("\033[2J\033[H")  /* ANSI clear */
    
    /* Fixed-width types for standard platforms */
    typedef int32_t sql_int_t;
    typedef uint16_t sql_uint16_t;
#endif

/* Memory constraints for UNIVAC (40kB total) */
#define MAX_RECORDS 118
#define MAX_NAME_LEN 14
#define MAX_SYMBOL_LEN 4
#define MAX_SERIES_LEN 22
#define MAX_SQL_LEN 256
#define MAX_TOKENS 20
#define MAX_TOKEN_LEN 32

/* Table structure - Periodic Table of Elements Database */
typedef struct {
    sql_int_t element_number;
    char element_name[MAX_NAME_LEN];
    char symbol[MAX_SYMBOL_LEN];
    double atomic_weight;
    char series[MAX_SERIES_LEN];
    sql_int_t active;               /* 1=active, 0=deleted */
} ElementRecord;

/* SQL Token Types */
typedef enum {
    TOKEN_SELECT,
    TOKEN_INSERT,
    TOKEN_DELETE,
    TOKEN_UPDATE,
    TOKEN_WHERE,
    TOKEN_FROM,
    TOKEN_INTO,
    TOKEN_VALUES,
    TOKEN_SET,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_DISTINCT,
    TOKEN_ORDER,
    TOKEN_BY,
    TOKEN_LIMIT,
    TOKEN_OFFSET,
    TOKEN_FETCH,
    TOKEN_FIRST,
    TOKEN_NEXT,
    TOKEN_ROWS,
    TOKEN_ONLY,
    TOKEN_LIKE,
    TOKEN_IN,
    TOKEN_BETWEEN,
    TOKEN_ASC,
    TOKEN_DESC,
    TOKEN_ASTERISK,
    TOKEN_EQUALS,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_GTE,
    TOKEN_LTE,
    TOKEN_NE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_UNKNOWN,
    TOKEN_EOF
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    char value[MAX_TOKEN_LEN];
} Token;

/* SQL Parser state */
typedef struct {
    Token tokens[MAX_TOKENS];
    sql_int_t token_count;
    sql_int_t current_token;
    sql_int_t use_distinct;      /* Flag for DISTINCT */
    sql_int_t limit_count;       /* LIMIT value (-1 = no limit) */
    sql_int_t offset_count;      /* OFFSET value (0 = no offset) */
    sql_int_t use_order_by;      /* Flag for ORDER BY */
    char order_field[MAX_TOKEN_LEN];  /* Field to order by */
    sql_int_t order_desc;        /* 1=DESC, 0=ASC */
} Parser;

/* Function prototypes - Database operations */
void init_database(void);
void load_demo_data(void);
int get_active_count(void);
void generate_random_name(char *dest, int max_len);

/* Function prototypes - SQL Engine */
void execute_sql(const char *sql);
void tokenize_sql(const char *sql, Parser *parser);
void parse_and_execute(Parser *parser);
void execute_select(Parser *parser);
void execute_insert(Parser *parser);
void execute_delete(Parser *parser);
void execute_update(Parser *parser);

/* Function prototypes - Query evaluation */
int evaluate_where_clause(Parser *parser, ElementRecord *rec, int *pos);
int evaluate_condition(const char *field, const char *op, const char *value, ElementRecord *rec);
int pattern_match(const char *text, const char *pattern);
int get_field_value(const char *field, ElementRecord *rec);
double get_field_double(const char *field, ElementRecord *rec);
void get_field_string(const char *field, ElementRecord *rec, char *dest, int max_len);

/* Function prototypes - Display */
void print_header(void);
void print_record(ElementRecord *rec);
void print_separator(void);
void print_full_record(ElementRecord *rec);

/* Function prototypes - Utilities */
void to_upper(char *str);
int is_keyword(const char *word);
void skip_whitespace(const char **p);
const char* get_next_token(const char *sql, char *token, int max_len);

/* Global database */
extern ElementRecord g_database[MAX_RECORDS];
extern sql_int_t g_record_count;

#endif /* SQLSTEEL_H */
