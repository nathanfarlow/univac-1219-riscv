/*
 * Query Executor Module
 * Handles SELECT, INSERT, DELETE, UPDATE operations
 */

#include "sqlsteel.h"

/* Get integer field value from record */
int get_field_value(const char *field, ElementRecord *rec) {
    if (strcmp(field, "ELEMENT_NUMBER") == 0 || strcmp(field, "NUM") == 0) return rec->element_number;
    return 0;
}

/* Get double field value from record */
double get_field_double(const char *field, ElementRecord *rec) {
    if (strcmp(field, "ATOMIC_WEIGHT") == 0 || strcmp(field, "WEIGHT") == 0) return rec->atomic_weight;
    return 0.0;
}

/* Get string field value from record */
void get_field_string(const char *field, ElementRecord *rec, char *dest, int max_len) {
    if (strcmp(field, "ELEMENT_NAME") == 0 || strcmp(field, "NAME") == 0) {
        PLATFORM_STRCPY(dest, max_len, rec->element_name);
    } else if (strcmp(field, "SYMBOL") == 0) {
        PLATFORM_STRCPY(dest, max_len, rec->symbol);
    } else if (strcmp(field, "SERIES") == 0) {
        PLATFORM_STRCPY(dest, max_len, rec->series);
    } else {
        dest[0] = '\0';
    }
}

/* Evaluate a single condition */
int evaluate_condition(const char *field, const char *op, const char *value, ElementRecord *rec) {
    int field_val, comp_val;
    double field_dbl, comp_dbl;
    char field_str[64];
    char value_upper[64];
    
    /* Check if this is an integer field */
    if (strcmp(field, "ELEMENT_NUMBER") == 0 || strcmp(field, "NUM") == 0) {
        field_val = get_field_value(field, rec);
        comp_val = atoi(value);
        if (strcmp(op, "=") == 0) return field_val == comp_val;
        if (strcmp(op, ">") == 0) return field_val > comp_val;
        if (strcmp(op, "<") == 0) return field_val < comp_val;
        if (strcmp(op, ">=") == 0) return field_val >= comp_val;
        if (strcmp(op, "<=") == 0) return field_val <= comp_val;
        if (strcmp(op, "<>") == 0 || strcmp(op, "!=") == 0) return field_val != comp_val;
    }
    /* Check if this is a double field */
    else if (strcmp(field, "ATOMIC_WEIGHT") == 0 || strcmp(field, "WEIGHT") == 0) {
        field_dbl = get_field_double(field, rec);
        comp_dbl = atof(value);
        if (strcmp(op, "=") == 0) return field_dbl == comp_dbl;
        if (strcmp(op, ">") == 0) return field_dbl > comp_dbl;
        if (strcmp(op, "<") == 0) return field_dbl < comp_dbl;
        if (strcmp(op, ">=") == 0) return field_dbl >= comp_dbl;
        if (strcmp(op, "<=") == 0) return field_dbl <= comp_dbl;
        if (strcmp(op, "<>") == 0 || strcmp(op, "!=") == 0) return field_dbl != comp_dbl;
    }
    /* String comparison */
    else {
        get_field_string(field, rec, field_str, sizeof(field_str));
        PLATFORM_STRCPY(value_upper, sizeof(value_upper), value);
        to_upper(value_upper);
        
        if (strcmp(op, "=") == 0) {
            return strcmp(field_str, value_upper) == 0;
        }
        if (strcmp(op, "<>") == 0 || strcmp(op, "!=") == 0) {
            return strcmp(field_str, value_upper) != 0;
        }
        /* LIKE operator - simple wildcard matching */
        if (strcmp(op, "LIKE") == 0) {
            return pattern_match(field_str, value_upper);
        }
    }
    
    return 0;
}

/* Simple pattern matching for LIKE operator */
/* Supports % (any chars) and _ (single char) */
int pattern_match(const char *text, const char *pattern) {
    const char *t = text;
    const char *p = pattern;
    const char *star_text = NULL;
    const char *star_pattern = NULL;
    
    while (*t) {
        if (*p == '%') {
            star_text = t;
            star_pattern = ++p;
        } else if (*p == '_') {
            t++;
            p++;
        } else if (*p == *t) {
            t++;
            p++;
        } else if (star_pattern) {
            p = star_pattern;
            t = ++star_text;
        } else {
            return 0;
        }
    }
    
    while (*p == '%') p++;
    return *p == '\0';
}

/* Evaluate WHERE clause */
int evaluate_where_clause(Parser *parser, ElementRecord *rec, int *pos) {
    int result = 1;
    int i = *pos;
    int current_result = 1;
    int negate_next = 0;
    int use_or = 0;
    
    /* Complex WHERE evaluation with AND, OR, NOT, IN, BETWEEN, LIKE */
    while (i < parser->token_count) {
        if (parser->tokens[i].type == TOKEN_SEMICOLON) break;
        if (parser->tokens[i].type == TOKEN_ORDER) break;
        if (parser->tokens[i].type == TOKEN_LIMIT) break;
        if (parser->tokens[i].type == TOKEN_OFFSET) break;
        if (parser->tokens[i].type == TOKEN_FETCH) break;
        
        /* Handle NOT */
        if (parser->tokens[i].type == TOKEN_NOT) {
            negate_next = 1;
            i++;
            continue;
        }
        
        /* Handle AND/OR */
        if (parser->tokens[i].type == TOKEN_AND) {
            if (use_or) {
                result = result || current_result;
                use_or = 0;
            }
            i++;
            continue;
        }
        if (parser->tokens[i].type == TOKEN_OR) {
            if (!use_or) {
                result = result && current_result;
            }
            use_or = 1;
            i++;
            continue;
        }
        
        if (parser->tokens[i].type == TOKEN_IDENTIFIER) {
            char field[MAX_TOKEN_LEN];
            char op[8];
            char value[MAX_TOKEN_LEN];
            int cond_result = 0;
            
            PLATFORM_STRCPY(field, MAX_TOKEN_LEN, parser->tokens[i].value);
            i++;
            
            if (i >= parser->token_count) break;
            
            /* Handle IN operator */
            if (parser->tokens[i].type == TOKEN_IN) {
                i++; /* Skip IN */
                if (i < parser->token_count && parser->tokens[i].type == TOKEN_LPAREN) {
                    i++; /* Skip ( */
                    cond_result = 0;
                    
                    /* Check if field matches any value in list */
                    while (i < parser->token_count && parser->tokens[i].type != TOKEN_RPAREN) {
                        if (parser->tokens[i].type == TOKEN_STRING || 
                            parser->tokens[i].type == TOKEN_NUMBER ||
                            parser->tokens[i].type == TOKEN_IDENTIFIER) {
                            if (evaluate_condition(field, "=", parser->tokens[i].value, rec)) {
                                cond_result = 1;
                                break;
                            }
                        }
                        i++;
                    }
                    
                    /* Skip to ) */
                    while (i < parser->token_count && parser->tokens[i].type != TOKEN_RPAREN) i++;
                    if (i < parser->token_count) i++; /* Skip ) */
                }
            }
            /* Handle BETWEEN operator */
            else if (parser->tokens[i].type == TOKEN_BETWEEN) {
                i++; /* Skip BETWEEN */
                if (i + 2 < parser->token_count) {
                    char low_val[MAX_TOKEN_LEN];
                    char high_val[MAX_TOKEN_LEN];
                    
                    PLATFORM_STRCPY(low_val, MAX_TOKEN_LEN, parser->tokens[i].value);
                    i++; /* Move to AND */
                    
                    if (parser->tokens[i].type == TOKEN_AND) {
                        i++; /* Skip AND */
                        PLATFORM_STRCPY(high_val, MAX_TOKEN_LEN, parser->tokens[i].value);
                        i++;
                        
                        /* Check if field is between low and high */
                        cond_result = evaluate_condition(field, ">=", low_val, rec) &&
                                      evaluate_condition(field, "<=", high_val, rec);
                    }
                }
            }
            /* Handle regular operators */
            else {
                /* Get operator */
                if (parser->tokens[i].type == TOKEN_EQUALS) strcpy(op, "=");
                else if (parser->tokens[i].type == TOKEN_GT) strcpy(op, ">");
                else if (parser->tokens[i].type == TOKEN_LT) strcpy(op, "<");
                else if (parser->tokens[i].type == TOKEN_GTE) strcpy(op, ">=");
                else if (parser->tokens[i].type == TOKEN_LTE) strcpy(op, "<=");
                else if (parser->tokens[i].type == TOKEN_NE) strcpy(op, "<>");
                else if (parser->tokens[i].type == TOKEN_LIKE) strcpy(op, "LIKE");
                else break;
                i++;
                
                if (i >= parser->token_count) break;
                
                /* Get value */
                if (parser->tokens[i].type == TOKEN_NUMBER || 
                    parser->tokens[i].type == TOKEN_STRING ||
                    parser->tokens[i].type == TOKEN_IDENTIFIER) {
                    PLATFORM_STRCPY(value, MAX_TOKEN_LEN, parser->tokens[i].value);
                    i++;
                } else break;
                
                /* Evaluate condition */
                cond_result = evaluate_condition(field, op, value, rec);
            }
            
            /* Apply negation if needed */
            if (negate_next) {
                cond_result = !cond_result;
                negate_next = 0;
            }
            
            /* Combine with previous results */
            if (use_or) {
                current_result = cond_result;
            } else {
                current_result = current_result && cond_result;
            }
        } else {
            i++;
        }
    }
    
    /* Final combination */
    if (use_or) {
        result = result || current_result;
    } else {
        result = result && current_result;
    }
    
    *pos = i;
    return result;
}

/* Helper: Compare two records for sorting */
int compare_records(const void *a, const void *b, const char *field, int desc) {
    const ElementRecord *r1 = (const ElementRecord *)a;
    const ElementRecord *r2 = (const ElementRecord *)b;
    int result = 0;
    
    if (strcmp(field, "ELEMENT_NUMBER") == 0 || strcmp(field, "NUM") == 0) {
        result = r1->element_number - r2->element_number;
    } else if (strcmp(field, "ATOMIC_WEIGHT") == 0 || strcmp(field, "WEIGHT") == 0) {
        result = (r1->atomic_weight > r2->atomic_weight) ? 1 : ((r1->atomic_weight < r2->atomic_weight) ? -1 : 0);
    } else if (strcmp(field, "ELEMENT_NAME") == 0 || strcmp(field, "NAME") == 0) {
        result = strcmp(r1->element_name, r2->element_name);
    } else if (strcmp(field, "SYMBOL") == 0) {
        result = strcmp(r1->symbol, r2->symbol);
    } else if (strcmp(field, "SERIES") == 0) {
        result = strcmp(r1->series, r2->series);
    }
    
    return desc ? -result : result;
}

/* Quick sort implementation for records */
void sort_records(ElementRecord *arr, int left, int right, const char *field, int desc) {
    if (left >= right) return;
    
    int i = left, j = right;
    ElementRecord pivot = arr[(left + right) / 2];
    ElementRecord temp;
    
    while (i <= j) {
        while (compare_records(&arr[i], &pivot, field, desc) < 0) i++;
        while (compare_records(&arr[j], &pivot, field, desc) > 0) j--;
        
        if (i <= j) {
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
            i++;
            j--;
        }
    }
    
    if (left < j) sort_records(arr, left, j, field, desc);
    if (i < right) sort_records(arr, i, right, field, desc);
}

/* Check if record is duplicate in result set */
int is_duplicate(ElementRecord *results, int count, ElementRecord *rec) {
    int i;
    for (i = 0; i < count; i++) {
        if (strcmp(results[i].element_name, rec->element_name) == 0 &&
            strcmp(results[i].symbol, rec->symbol) == 0 &&
            results[i].atomic_weight == rec->atomic_weight) {
            return 1;
        }
    }
    return 0;
}

/* Execute SELECT statement */
void execute_select(Parser *parser) {
    int i, count = 0;
    int has_where = 0;
    int where_pos = 0;
    ElementRecord results[MAX_RECORDS];
    int result_count = 0;
    
    /* Initialize parser state */
    parser->use_distinct = 0;
    parser->limit_count = -1;
    parser->offset_count = 0;
    parser->use_order_by = 0;
    parser->order_desc = 0;
    
    /* Parse SELECT modifiers */
    i = 1; /* Skip SELECT token */
    if (i < parser->token_count && parser->tokens[i].type == TOKEN_DISTINCT) {
        parser->use_distinct = 1;
        i++;
    }
    
    /* Find WHERE clause */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_WHERE) {
            has_where = 1;
            where_pos = i + 1;
            break;
        }
    }
    
    /* Find ORDER BY clause */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_ORDER && 
            i + 2 < parser->token_count &&
            parser->tokens[i + 1].type == TOKEN_BY) {
            parser->use_order_by = 1;
            PLATFORM_STRCPY(parser->order_field, MAX_TOKEN_LEN, parser->tokens[i + 2].value);
            
            /* Check for ASC/DESC */
            if (i + 3 < parser->token_count) {
                if (parser->tokens[i + 3].type == TOKEN_DESC) {
                    parser->order_desc = 1;
                }
            }
            break;
        }
    }
    
    /* Find LIMIT clause */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_LIMIT && i + 1 < parser->token_count) {
            parser->limit_count = atoi(parser->tokens[i + 1].value);
            break;
        }
    }
    
    /* Find OFFSET clause */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_OFFSET && i + 1 < parser->token_count) {
            parser->offset_count = atoi(parser->tokens[i + 1].value);
            break;
        }
    }
    
    /* Find FETCH clause (SQL standard alternative to LIMIT) */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_FETCH) {
            /* FETCH FIRST n ROWS ONLY or FETCH NEXT n ROWS ONLY */
            if (i + 1 < parser->token_count && 
                (parser->tokens[i + 1].type == TOKEN_FIRST || 
                 parser->tokens[i + 1].type == TOKEN_NEXT)) {
                if (i + 2 < parser->token_count && 
                    parser->tokens[i + 2].type == TOKEN_NUMBER) {
                    parser->limit_count = atoi(parser->tokens[i + 2].value);
                }
            }
            break;
        }
    }
    
    /* Collect matching records */
    for (i = 0; i < MAX_RECORDS; i++) {
        if (g_database[i].active) {
            int matches = 1;
            
            if (has_where) {
                int pos = where_pos;
                matches = evaluate_where_clause(parser, &g_database[i], &pos);
            }
            
            if (matches) {
                /* Check for DISTINCT */
                if (parser->use_distinct && is_duplicate(results, result_count, &g_database[i])) {
                    continue;
                }
                
                results[result_count++] = g_database[i];
            }
        }
    }
    
    /* Apply ORDER BY */
    if (parser->use_order_by && result_count > 0) {
        sort_records(results, 0, result_count - 1, parser->order_field, parser->order_desc);
    }
    
    /* Apply OFFSET and LIMIT */
    print_header();
    
    for (i = 0; i < result_count; i++) {
        /* Skip OFFSET records */
        if (i < parser->offset_count) continue;
        
        /* Stop at LIMIT */
        if (parser->limit_count >= 0 && count >= parser->limit_count) break;
        
        print_record(&results[i]);
        count++;
    }
    
    print_separator();
    printf("RECORDS RETURNED: %d", count);
    if (parser->use_distinct) printf(" (DISTINCT)");
    if (parser->offset_count > 0) printf(" (OFFSET %d)", parser->offset_count);
    if (parser->limit_count >= 0) printf(" (LIMIT %d)", parser->limit_count);
    printf("\n");
}

/* Execute INSERT statement */
void execute_insert(Parser *parser) {
    int i, slot = -1;
    ElementRecord new_rec;
    
    /* Find empty slot */
    for (i = 0; i < MAX_RECORDS; i++) {
        if (!g_database[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf("ERROR: DATABASE FULL (MAX %d RECORDS)\n", MAX_RECORDS);
        return;
    }
    
    /* Parse INSERT INTO ELEMENTS VALUES (...) */
    /* Simplified: expects 4 values in order: name, symbol, weight, series */
    int value_start = 0;
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_VALUES) {
            value_start = i + 1;
            break;
        }
    }
    
    if (value_start == 0) {
        printf("ERROR: INVALID INSERT SYNTAX\n");
        printf("USAGE: INSERT INTO ELEMENTS VALUES ('ELEMENT_NAME', 'SYMBOL', WEIGHT, 'SERIES')\n");
        return;
    }
    
    /* Skip opening paren */
    if (parser->tokens[value_start].type == TOKEN_LPAREN) value_start++;
    
    /* Parse values */
    int val_idx = value_start;
    int field_num = 0;
    
    new_rec.active = 1;
    new_rec.element_number = slot + 1;
    
    while (val_idx < parser->token_count && field_num < 4) {
        if (parser->tokens[val_idx].type == TOKEN_COMMA) {
            val_idx++;
            continue;
        }
        if (parser->tokens[val_idx].type == TOKEN_RPAREN) break;
        
        switch (field_num) {
            case 0: /* ELEMENT_NAME */
                PLATFORM_STRCPY(new_rec.element_name, MAX_NAME_LEN, parser->tokens[val_idx].value);
                break;
            case 1: /* SYMBOL */
                PLATFORM_STRCPY(new_rec.symbol, MAX_SYMBOL_LEN, parser->tokens[val_idx].value);
                break;
            case 2: /* ATOMIC_WEIGHT */
                new_rec.atomic_weight = atof(parser->tokens[val_idx].value);
                break;
            case 3: /* SERIES */
                PLATFORM_STRCPY(new_rec.series, MAX_SERIES_LEN, parser->tokens[val_idx].value);
                break;
        }
        
        field_num++;
        val_idx++;
    }
    
    if (field_num == 4) {
        g_database[slot] = new_rec;
        g_record_count++;
        printf("ELEMENT INSERTED: NUM=%d (NAME: %s, SYMBOL: %s)\n", new_rec.element_number, new_rec.element_name, new_rec.symbol);
    } else {
        printf("ERROR: INSUFFICIENT VALUES (EXPECTED 4, GOT %d)\n", field_num);
        printf("USAGE: INSERT INTO ELEMENTS VALUES ('ELEMENT_NAME', 'SYMBOL', WEIGHT, 'SERIES')\n");
    }
}

/* Execute DELETE statement */
void execute_delete(Parser *parser) {
    int i, count = 0;
    int has_where = 0;
    int where_pos = 0;
    
    /* Find WHERE clause */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_WHERE) {
            has_where = 1;
            where_pos = i + 1;
            break;
        }
    }
    
    if (!has_where) {
        printf("ERROR: DELETE REQUIRES WHERE CLAUSE (SAFETY)\n");
        printf("USAGE: DELETE FROM ELEMENTS WHERE <condition>\n");
        return;
    }
    
    for (i = 0; i < MAX_RECORDS; i++) {
        if (g_database[i].active) {
            int pos = where_pos;
            if (evaluate_where_clause(parser, &g_database[i], &pos)) {
                g_database[i].active = 0;
                g_record_count--;
                count++;
            }
        }
    }
    
    printf("RECORDS DELETED: %d\n", count);
}

/* Execute UPDATE statement */
void execute_update(Parser *parser) {
    int i, count = 0;
    int set_pos = 0, where_pos = 0;
    int has_where = 0;
    char update_field[MAX_TOKEN_LEN];
    char update_value[MAX_TOKEN_LEN];
    
    /* Find SET and WHERE clauses */
    for (i = 0; i < parser->token_count; i++) {
        if (parser->tokens[i].type == TOKEN_SET) {
            set_pos = i + 1;
        }
        if (parser->tokens[i].type == TOKEN_WHERE) {
            has_where = 1;
            where_pos = i + 1;
            break;
        }
    }
    
    if (set_pos == 0) {
        printf("ERROR: UPDATE REQUIRES SET CLAUSE\n");
        printf("USAGE: UPDATE ELEMENTS SET FIELD=VALUE WHERE <condition>\n");
        return;
    }
    
    /* Parse SET clause (simplified: single field) */
    if (parser->tokens[set_pos].type == TOKEN_IDENTIFIER &&
        set_pos + 1 < parser->token_count &&
        parser->tokens[set_pos + 1].type == TOKEN_EQUALS &&
        set_pos + 2 < parser->token_count) {
        
        PLATFORM_STRCPY(update_field, MAX_TOKEN_LEN, parser->tokens[set_pos].value);
        PLATFORM_STRCPY(update_value, MAX_TOKEN_LEN, parser->tokens[set_pos + 2].value);
    } else {
        printf("ERROR: INVALID SET SYNTAX\n");
        return;
    }
    
    /* Update matching records */
    for (i = 0; i < MAX_RECORDS; i++) {
        if (g_database[i].active) {
            int matches = 1;
            
            if (has_where) {
                int pos = where_pos;
                matches = evaluate_where_clause(parser, &g_database[i], &pos);
            }
            
            if (matches) {
                /* Apply update */
                if (strcmp(update_field, "ATOMIC_WEIGHT") == 0 || strcmp(update_field, "WEIGHT") == 0) {
                    g_database[i].atomic_weight = atof(update_value);
                } else if (strcmp(update_field, "ELEMENT_NAME") == 0 || strcmp(update_field, "NAME") == 0) {
                    PLATFORM_STRCPY(g_database[i].element_name, MAX_NAME_LEN, update_value);
                } else if (strcmp(update_field, "SYMBOL") == 0) {
                    PLATFORM_STRCPY(g_database[i].symbol, MAX_SYMBOL_LEN, update_value);
                } else if (strcmp(update_field, "SERIES") == 0) {
                    PLATFORM_STRCPY(g_database[i].series, MAX_SERIES_LEN, update_value);
                }
                count++;
            }
        }
    }
    
    printf("RECORDS UPDATED: %d\n", count);
}
