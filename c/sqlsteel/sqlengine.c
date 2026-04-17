/*
 * SQL Engine Module
 * Tokenizer, parser, and query executor
 */

#include "sqlsteel.h"

/* Convert string to uppercase */
void to_upper(char *str) {
    while (*str) {
        *str = (char)toupper((unsigned char)*str);
        str++;
    }
}

/* Check if word is a SQL keyword */
int is_keyword(const char *word) {
    const char *keywords[] = {
        "SELECT", "INSERT", "DELETE", "UPDATE", "WHERE", "FROM", 
        "INTO", "VALUES", "SET", "AND", "OR", "NOT", "DISTINCT",
        "ORDER", "BY", "LIMIT", "OFFSET", "FETCH", "FIRST", "NEXT",
        "ROWS", "ONLY", "LIKE", "IN", "BETWEEN", "ASC", "DESC", NULL
    };
    int i;
    for (i = 0; keywords[i] != NULL; i++) {
        if (strcmp(word, keywords[i]) == 0) return 1;
    }
    return 0;
}

/* Skip whitespace in string */
void skip_whitespace(const char **p) {
    while (**p && isspace((unsigned char)**p)) {
        (*p)++;
    }
}

/* Tokenize SQL command */
void tokenize_sql(const char *sql, Parser *parser) {
    const char *p = sql;
    int token_idx = 0;
    
    parser->token_count = 0;
    parser->current_token = 0;
    
    while (*p && token_idx < MAX_TOKENS) {
        skip_whitespace(&p);
        if (!*p) break;
        
        Token *tok = &parser->tokens[token_idx];
        
        /* Handle string literals */
        if (*p == '\'' || *p == '"') {
            char quote = *p;
            int i = 0;
            p++;
            while (*p && *p != quote && i < MAX_TOKEN_LEN - 1) {
                tok->value[i++] = *p++;
            }
            tok->value[i] = '\0';
            if (*p == quote) p++;
            tok->type = TOKEN_STRING;
            token_idx++;
        }
        /* Handle numbers */
        else if (isdigit((unsigned char)*p)) {
            int i = 0;
            while (isdigit((unsigned char)*p) && i < MAX_TOKEN_LEN - 1) {
                tok->value[i++] = *p++;
            }
            tok->value[i] = '\0';
            tok->type = TOKEN_NUMBER;
            token_idx++;
        }
        /* Handle operators and special characters */
        else if (*p == '*') {
            tok->value[0] = '*'; tok->value[1] = '\0';
            tok->type = TOKEN_ASTERISK;
            p++; token_idx++;
        }
        else if (*p == '=') {
            tok->value[0] = '='; tok->value[1] = '\0';
            tok->type = TOKEN_EQUALS;
            p++; token_idx++;
        }
        else if (*p == '>' && *(p+1) == '=') {
            tok->value[0] = '>'; tok->value[1] = '='; tok->value[2] = '\0';
            tok->type = TOKEN_GTE;
            p += 2; token_idx++;
        }
        else if (*p == '<' && *(p+1) == '=') {
            tok->value[0] = '<'; tok->value[1] = '='; tok->value[2] = '\0';
            tok->type = TOKEN_LTE;
            p += 2; token_idx++;
        }
        else if (*p == '<' && *(p+1) == '>') {
            tok->value[0] = '<'; tok->value[1] = '>'; tok->value[2] = '\0';
            tok->type = TOKEN_NE;
            p += 2; token_idx++;
        }
        else if (*p == '!' && *(p+1) == '=') {
            tok->value[0] = '!'; tok->value[1] = '='; tok->value[2] = '\0';
            tok->type = TOKEN_NE;
            p += 2; token_idx++;
        }
        else if (*p == '>') {
            tok->value[0] = '>'; tok->value[1] = '\0';
            tok->type = TOKEN_GT;
            p++; token_idx++;
        }
        else if (*p == '<') {
            tok->value[0] = '<'; tok->value[1] = '\0';
            tok->type = TOKEN_LT;
            p++; token_idx++;
        }
        else if (*p == '(') {
            tok->value[0] = '('; tok->value[1] = '\0';
            tok->type = TOKEN_LPAREN;
            p++; token_idx++;
        }
        else if (*p == ')') {
            tok->value[0] = ')'; tok->value[1] = '\0';
            tok->type = TOKEN_RPAREN;
            p++; token_idx++;
        }
        else if (*p == ',') {
            tok->value[0] = ','; tok->value[1] = '\0';
            tok->type = TOKEN_COMMA;
            p++; token_idx++;
        }
        else if (*p == ';') {
            tok->value[0] = ';'; tok->value[1] = '\0';
            tok->type = TOKEN_SEMICOLON;
            p++; token_idx++;
        }
        /* Handle identifiers and keywords */
        else if (isalpha((unsigned char)*p) || *p == '_') {
            int i = 0;
            while ((isalnum((unsigned char)*p) || *p == '_') && i < MAX_TOKEN_LEN - 1) {
                tok->value[i++] = *p++;
            }
            tok->value[i] = '\0';
            to_upper(tok->value);
            
            /* Classify as keyword or identifier */
            if (strcmp(tok->value, "SELECT") == 0) tok->type = TOKEN_SELECT;
            else if (strcmp(tok->value, "INSERT") == 0) tok->type = TOKEN_INSERT;
            else if (strcmp(tok->value, "DELETE") == 0) tok->type = TOKEN_DELETE;
            else if (strcmp(tok->value, "UPDATE") == 0) tok->type = TOKEN_UPDATE;
            else if (strcmp(tok->value, "WHERE") == 0) tok->type = TOKEN_WHERE;
            else if (strcmp(tok->value, "FROM") == 0) tok->type = TOKEN_FROM;
            else if (strcmp(tok->value, "INTO") == 0) tok->type = TOKEN_INTO;
            else if (strcmp(tok->value, "VALUES") == 0) tok->type = TOKEN_VALUES;
            else if (strcmp(tok->value, "SET") == 0) tok->type = TOKEN_SET;
            else if (strcmp(tok->value, "AND") == 0) tok->type = TOKEN_AND;
            else if (strcmp(tok->value, "OR") == 0) tok->type = TOKEN_OR;
            else if (strcmp(tok->value, "NOT") == 0) tok->type = TOKEN_NOT;
            else if (strcmp(tok->value, "DISTINCT") == 0) tok->type = TOKEN_DISTINCT;
            else if (strcmp(tok->value, "ORDER") == 0) tok->type = TOKEN_ORDER;
            else if (strcmp(tok->value, "BY") == 0) tok->type = TOKEN_BY;
            else if (strcmp(tok->value, "LIMIT") == 0) tok->type = TOKEN_LIMIT;
            else if (strcmp(tok->value, "OFFSET") == 0) tok->type = TOKEN_OFFSET;
            else if (strcmp(tok->value, "FETCH") == 0) tok->type = TOKEN_FETCH;
            else if (strcmp(tok->value, "FIRST") == 0) tok->type = TOKEN_FIRST;
            else if (strcmp(tok->value, "NEXT") == 0) tok->type = TOKEN_NEXT;
            else if (strcmp(tok->value, "ROWS") == 0) tok->type = TOKEN_ROWS;
            else if (strcmp(tok->value, "ONLY") == 0) tok->type = TOKEN_ONLY;
            else if (strcmp(tok->value, "LIKE") == 0) tok->type = TOKEN_LIKE;
            else if (strcmp(tok->value, "IN") == 0) tok->type = TOKEN_IN;
            else if (strcmp(tok->value, "BETWEEN") == 0) tok->type = TOKEN_BETWEEN;
            else if (strcmp(tok->value, "ASC") == 0) tok->type = TOKEN_ASC;
            else if (strcmp(tok->value, "DESC") == 0) tok->type = TOKEN_DESC;
            else tok->type = TOKEN_IDENTIFIER;
            
            token_idx++;
        }
        else {
            p++; /* Skip unknown character */
        }
    }
    
    parser->token_count = token_idx;
}

/* Execute SQL command */
void execute_sql(const char *sql) {
    Parser parser;
    
    if (strlen(sql) == 0) return;
    
    tokenize_sql(sql, &parser);
    
    if (parser.token_count == 0) {
        printf("ERROR: EMPTY COMMAND\n");
        return;
    }
    
    parse_and_execute(&parser);
}

/* Parse and execute based on command type */
void parse_and_execute(Parser *parser) {
    if (parser->token_count == 0) return;
    
    switch (parser->tokens[0].type) {
        case TOKEN_SELECT:
            execute_select(parser);
            break;
        case TOKEN_INSERT:
            execute_insert(parser);
            break;
        case TOKEN_DELETE:
            execute_delete(parser);
            break;
        case TOKEN_UPDATE:
            execute_update(parser);
            break;
        default:
            printf("ERROR: UNKNOWN COMMAND '%s'\n", parser->tokens[0].value);
            break;
    }
}
