#include "symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *xstrdup(const char *s) {
    char *copy = malloc(strlen(s) + 1);
    if (!copy) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(copy, s);
    return copy;
}

static int is_ident_char(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static char *make_c_name(SymbolTable *table, const char *name) {
    size_t len = strlen(name);
    char *safe = malloc(len + 1);
    if (!safe) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    for (size_t i = 0; i < len; i++) {
        safe[i] = is_ident_char(name[i]) ? name[i] : '_';
    }
    safe[len] = '\0';

    int size = snprintf(NULL, 0, "mc_%d_%d_%s", table->scope_depth, table->next_id, safe);
    char *out = malloc((size_t)size + 1);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    snprintf(out, (size_t)size + 1, "mc_%d_%d_%s", table->scope_depth, table->next_id, safe);
    free(safe);
    return out;
}

SymbolTable *symbol_table_new(void) {
    SymbolTable *table = calloc(1, sizeof(SymbolTable));
    if (!table) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    table->scope_depth = 0;
    table->next_address = 0;
    table->next_id = 0;
    return table;
}

void symbol_table_free(SymbolTable *table) {
    if (!table) return;
    Symbol *sym = table->head;
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym->c_name);
        free(sym);
        sym = next;
    }
    free(table);
}

void symbol_table_enter_scope(SymbolTable *table) {
    table->scope_depth++;
}

void symbol_table_exit_scope(SymbolTable *table) {
    for (Symbol *sym = table->head; sym; sym = sym->next) {
        if (sym->scope_depth == table->scope_depth) {
            sym->active = 0;
        }
    }
    if (table->scope_depth > 0) table->scope_depth--;
}

Symbol *symbol_table_lookup_current(SymbolTable *table, const char *name) {
    for (Symbol *sym = table->head; sym; sym = sym->next) {
        if (sym->active && sym->scope_depth == table->scope_depth && strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

Symbol *symbol_table_lookup(SymbolTable *table, const char *name) {
    for (Symbol *sym = table->head; sym; sym = sym->next) {
        if (sym->active && strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

Symbol *symbol_table_declare(SymbolTable *table, const char *name, TypeKind type, int line) {
    if (symbol_table_lookup_current(table, name)) return NULL;
    Symbol *sym = calloc(1, sizeof(Symbol));
    if (!sym) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    sym->name = xstrdup(name);
    sym->type = type;
    sym->scope_depth = table->scope_depth;
    sym->line = line;
    sym->address = table->next_address++;
    sym->active = 1;
    sym->c_name = make_c_name(table, name);
    table->next_id++;
    sym->next = table->head;
    table->head = sym;
    return sym;
}

void symbol_table_print(SymbolTable *table) {
    printf("%-16s %-8s %-8s %-8s %-8s %s\n", "name", "type", "scope", "addr", "line", "c_name");
    printf("%-16s %-8s %-8s %-8s %-8s %s\n", "----", "----", "-----", "----", "----", "------");
    for (Symbol *sym = table->head; sym; sym = sym->next) {
        printf("%-16s %-8s %-8d %-8d %-8d %s\n",
               sym->name,
               type_to_string(sym->type),
               sym->scope_depth,
               sym->address,
               sym->line,
               sym->c_name);
    }
}
