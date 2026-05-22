#ifndef MINIC_TAC_H
#define MINIC_TAC_H

#include "ast.h"

typedef enum {
    TAC_DECL,
    TAC_ASSIGN,
    TAC_BINARY,
    TAC_UNARY,
    TAC_LABEL,
    TAC_GOTO,
    TAC_IF_FALSE,
    TAC_SCAN,
    TAC_PRINT
} TacKind;

typedef struct {
    TacKind kind;
    char *text;
    char *target;
    char *arg1;
    char *arg2;
    char *op;
    char *label;
    char *reads;
    char *writes;
    TypeKind value_type;
    int address;
    int active;
} TacInstr;

typedef struct {
    TacInstr *items;
    int count;
    int capacity;
    int temp_count;
    int label_count;
} TacList;

TacList *tac_generate(AstNode *program);
void tac_optimize(TacList *list);
void tac_print(TacList *list);
void tac_analyze_dependencies(TacList *list);
void tac_free(TacList *list);

#endif
