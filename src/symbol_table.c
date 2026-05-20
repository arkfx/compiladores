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

SymbolTable *symbol_table_new(void) {
    SymbolTable *table = calloc(1, sizeof(SymbolTable));
    if (!table) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    table->scope_depth = 0;
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
    sym->active = 1;
    sym->c_name = xstrdup(name);
    sym->next = table->head;
    table->head = sym;
    return sym;
}

void symbol_table_print(SymbolTable *table) {
    printf("%-20s %-8s %-8s %-8s\n", "name", "type", "scope", "line");
    printf("%-20s %-8s %-8s %-8s\n", "----", "----", "-----", "----");
    for (Symbol *sym = table->head; sym; sym = sym->next) {
        printf("%-20s %-8s %-8d %-8d\n",
               sym->name,
               type_to_string(sym->type),
               sym->scope_depth,
               sym->line);
    }
}
