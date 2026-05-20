#ifndef MINILANG_SYMBOL_TABLE_H
#define MINILANG_SYMBOL_TABLE_H

#include "ast.h"

typedef struct Symbol {
    char *name;
    TypeKind type;
    int scope_depth;
    int line;
    int active;
    char *c_name;
    struct Symbol *next;
} Symbol;

typedef struct {
    Symbol *head;
    int scope_depth;
} SymbolTable;

SymbolTable *symbol_table_new(void);
void symbol_table_free(SymbolTable *table);
void symbol_table_enter_scope(SymbolTable *table);
void symbol_table_exit_scope(SymbolTable *table);
Symbol *symbol_table_declare(SymbolTable *table, const char *name, TypeKind type, int line);
Symbol *symbol_table_lookup(SymbolTable *table, const char *name);
Symbol *symbol_table_lookup_current(SymbolTable *table, const char *name);
void symbol_table_print(SymbolTable *table);

#endif
