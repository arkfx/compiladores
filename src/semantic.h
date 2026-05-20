#ifndef MINILANG_SEMANTIC_H
#define MINILANG_SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

typedef struct {
    SymbolTable *symbols;
    int error_count;
} SemanticContext;

SemanticContext *semantic_context_new(void);
void semantic_context_free(SemanticContext *ctx);
int semantic_analyze(SemanticContext *ctx, AstNode *program);

#endif
