#ifndef MINILANG_TAC_H
#define MINILANG_TAC_H

#include "ast.h"

typedef struct {
    char *text;
    char *reads;
    char *writes;
} TacInstr;

typedef struct {
    TacInstr *items;
    int count;
    int capacity;
    int temp_count;
    int label_count;
} TacList;

TacList *tac_generate(AstNode *program);
void tac_print(TacList *list);
void tac_analyze_dependencies(TacList *list);
void tac_free(TacList *list);

#endif
