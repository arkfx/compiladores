#ifndef MINIC_AST_H
#define MINIC_AST_H

#include <stdio.h>

typedef enum {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_STRING_LITERAL,
    TYPE_VOID,
    TYPE_ERROR
} TypeKind;

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_DECL,
    AST_ASSIGN,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_SCAN,
    AST_PRINT,
    AST_BINARY,
    AST_UNARY,
    AST_INT_LITERAL,
    AST_BOOL_LITERAL,
    AST_STRING_LITERAL,
    AST_IDENTIFIER,
    AST_EMPTY
} AstKind;

typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD,
    BIN_EQ,
    BIN_NEQ,
    BIN_LT,
    BIN_LTE,
    BIN_GT,
    BIN_GTE,
    BIN_AND,
    BIN_OR
} BinOp;

typedef enum {
    UN_NEG,
    UN_NOT
} UnOp;

typedef struct AstNode AstNode;

typedef struct AstList {
    AstNode **items;
    int count;
    int capacity;
} AstList;

struct AstNode {
    AstKind kind;
    int line;
    TypeKind inferred_type;
    union {
        struct { AstList *statements; } program;
        struct { AstList *statements; } block;
        struct { TypeKind type; char *name; char *c_name; int address; AstNode *init; } decl;
        struct { char *name; char *c_name; AstNode *value; } assign;
        struct { AstNode *condition; AstNode *then_branch; AstNode *else_branch; } if_stmt;
        struct { AstNode *condition; AstNode *body; } while_stmt;
        struct { AstNode *init; AstNode *condition; AstNode *update; AstNode *body; } for_stmt;
        struct { char *name; char *c_name; TypeKind target_type; } scan;
        struct { AstNode *value; } print;
        struct { BinOp op; AstNode *left; AstNode *right; } binary;
        struct { UnOp op; AstNode *operand; } unary;
        int int_value;
        int bool_value;
        char *string_value;
        struct { char *name; char *c_name; } identifier;
    } as;
};

AstList *ast_list_new(void);
void ast_list_append(AstList *list, AstNode *node);

AstNode *ast_new_program(AstList *statements, int line);
AstNode *ast_new_block(AstList *statements, int line);
AstNode *ast_new_decl(TypeKind type, char *name, AstNode *init, int line);
AstNode *ast_new_assign(char *name, AstNode *value, int line);
AstNode *ast_new_if(AstNode *condition, AstNode *then_branch, AstNode *else_branch, int line);
AstNode *ast_new_while(AstNode *condition, AstNode *body, int line);
AstNode *ast_new_for(AstNode *init, AstNode *condition, AstNode *update, AstNode *body, int line);
AstNode *ast_new_scan(char *name, int line);
AstNode *ast_new_print(AstNode *value, int line);
AstNode *ast_new_binary(BinOp op, AstNode *left, AstNode *right, int line);
AstNode *ast_new_unary(UnOp op, AstNode *operand, int line);
AstNode *ast_new_int(int value, int line);
AstNode *ast_new_bool(int value, int line);
AstNode *ast_new_string(char *value, int line);
AstNode *ast_new_identifier(char *name, int line);
AstNode *ast_new_empty(int line);

const char *type_to_string(TypeKind type);
const char *binop_to_string(BinOp op);
const char *unop_to_string(UnOp op);
void ast_print(AstNode *node, int indent);
void ast_free(AstNode *node);

#endif
