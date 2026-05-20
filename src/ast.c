#include "ast.h"

#include <stdlib.h>
#include <string.h>

static AstNode *new_node(AstKind kind, int line) {
    AstNode *node = calloc(1, sizeof(AstNode));
    if (!node) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    node->kind = kind;
    node->line = line;
    node->inferred_type = TYPE_VOID;
    return node;
}

AstList *ast_list_new(void) {
    AstList *list = calloc(1, sizeof(AstList));
    if (!list) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    list->capacity = 8;
    list->items = calloc((size_t)list->capacity, sizeof(AstNode *));
    if (!list->items) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return list;
}

void ast_list_append(AstList *list, AstNode *node) {
    if (!list || !node) return;
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, (size_t)list->capacity * sizeof(AstNode *));
        if (!list->items) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    list->items[list->count++] = node;
}

AstNode *ast_new_program(AstList *statements, int line) {
    AstNode *node = new_node(AST_PROGRAM, line);
    node->as.program.statements = statements;
    return node;
}

AstNode *ast_new_block(AstList *statements, int line) {
    AstNode *node = new_node(AST_BLOCK, line);
    node->as.block.statements = statements;
    return node;
}

AstNode *ast_new_decl(TypeKind type, char *name, AstNode *init, int line) {
    AstNode *node = new_node(AST_DECL, line);
    node->as.decl.type = type;
    node->as.decl.name = name;
    node->as.decl.init = init;
    return node;
}

AstNode *ast_new_assign(char *name, AstNode *value, int line) {
    AstNode *node = new_node(AST_ASSIGN, line);
    node->as.assign.name = name;
    node->as.assign.value = value;
    return node;
}

AstNode *ast_new_if(AstNode *condition, AstNode *then_branch, AstNode *else_branch, int line) {
    AstNode *node = new_node(AST_IF, line);
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_branch = then_branch;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

AstNode *ast_new_while(AstNode *condition, AstNode *body, int line) {
    AstNode *node = new_node(AST_WHILE, line);
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

AstNode *ast_new_for(AstNode *init, AstNode *condition, AstNode *update, AstNode *body, int line) {
    AstNode *node = new_node(AST_FOR, line);
    node->as.for_stmt.init = init;
    node->as.for_stmt.condition = condition;
    node->as.for_stmt.update = update;
    node->as.for_stmt.body = body;
    return node;
}

AstNode *ast_new_scan(char *name, int line) {
    AstNode *node = new_node(AST_SCAN, line);
    node->as.scan.name = name;
    return node;
}

AstNode *ast_new_print(AstNode *value, int line) {
    AstNode *node = new_node(AST_PRINT, line);
    node->as.print.value = value;
    return node;
}

AstNode *ast_new_binary(BinOp op, AstNode *left, AstNode *right, int line) {
    AstNode *node = new_node(AST_BINARY, line);
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    return node;
}

AstNode *ast_new_unary(UnOp op, AstNode *operand, int line) {
    AstNode *node = new_node(AST_UNARY, line);
    node->as.unary.op = op;
    node->as.unary.operand = operand;
    return node;
}

AstNode *ast_new_int(int value, int line) {
    AstNode *node = new_node(AST_INT_LITERAL, line);
    node->as.int_value = value;
    node->inferred_type = TYPE_INT;
    return node;
}

AstNode *ast_new_bool(int value, int line) {
    AstNode *node = new_node(AST_BOOL_LITERAL, line);
    node->as.bool_value = value ? 1 : 0;
    node->inferred_type = TYPE_BOOL;
    return node;
}

AstNode *ast_new_string(char *value, int line) {
    AstNode *node = new_node(AST_STRING_LITERAL, line);
    node->as.string_value = value;
    node->inferred_type = TYPE_STRING_LITERAL;
    return node;
}

AstNode *ast_new_identifier(char *name, int line) {
    AstNode *node = new_node(AST_IDENTIFIER, line);
    node->as.identifier = name;
    return node;
}

AstNode *ast_new_empty(int line) {
    return new_node(AST_EMPTY, line);
}

const char *type_to_string(TypeKind type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING_LITERAL: return "string";
        case TYPE_VOID: return "void";
        case TYPE_ERROR: return "error";
        default: return "unknown";
    }
}

const char *binop_to_string(BinOp op) {
    switch (op) {
        case BIN_ADD: return "+";
        case BIN_SUB: return "-";
        case BIN_MUL: return "*";
        case BIN_DIV: return "/";
        case BIN_MOD: return "%";
        case BIN_EQ: return "==";
        case BIN_NEQ: return "!=";
        case BIN_LT: return "<";
        case BIN_LTE: return "<=";
        case BIN_GT: return ">";
        case BIN_GTE: return ">=";
        case BIN_AND: return "&&";
        case BIN_OR: return "||";
        default: return "?";
    }
}

const char *unop_to_string(UnOp op) {
    switch (op) {
        case UN_NEG: return "-";
        case UN_NOT: return "!";
        default: return "?";
    }
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void ast_print(AstNode *node, int indent) {
    if (!node) return;
    print_indent(indent);
    switch (node->kind) {
        case AST_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->as.program.statements->count; i++) {
                ast_print(node->as.program.statements->items[i], indent + 1);
            }
            break;
        case AST_BLOCK:
            printf("Block\n");
            for (int i = 0; i < node->as.block.statements->count; i++) {
                ast_print(node->as.block.statements->items[i], indent + 1);
            }
            break;
        case AST_DECL:
            printf("Decl %s %s\n", type_to_string(node->as.decl.type), node->as.decl.name);
            if (node->as.decl.init) ast_print(node->as.decl.init, indent + 1);
            break;
        case AST_ASSIGN:
            printf("Assign %s\n", node->as.assign.name);
            ast_print(node->as.assign.value, indent + 1);
            break;
        case AST_IF:
            printf("If\n");
            ast_print(node->as.if_stmt.condition, indent + 1);
            ast_print(node->as.if_stmt.then_branch, indent + 1);
            if (node->as.if_stmt.else_branch) ast_print(node->as.if_stmt.else_branch, indent + 1);
            break;
        case AST_WHILE:
            printf("While\n");
            ast_print(node->as.while_stmt.condition, indent + 1);
            ast_print(node->as.while_stmt.body, indent + 1);
            break;
        case AST_FOR:
            printf("For\n");
            ast_print(node->as.for_stmt.init, indent + 1);
            ast_print(node->as.for_stmt.condition, indent + 1);
            ast_print(node->as.for_stmt.update, indent + 1);
            ast_print(node->as.for_stmt.body, indent + 1);
            break;
        case AST_SCAN:
            printf("Scan %s\n", node->as.scan.name);
            break;
        case AST_PRINT:
            printf("Print\n");
            ast_print(node->as.print.value, indent + 1);
            break;
        case AST_BINARY:
            printf("Binary %s : %s\n", binop_to_string(node->as.binary.op), type_to_string(node->inferred_type));
            ast_print(node->as.binary.left, indent + 1);
            ast_print(node->as.binary.right, indent + 1);
            break;
        case AST_UNARY:
            printf("Unary %s : %s\n", unop_to_string(node->as.unary.op), type_to_string(node->inferred_type));
            ast_print(node->as.unary.operand, indent + 1);
            break;
        case AST_INT_LITERAL:
            printf("Int %d\n", node->as.int_value);
            break;
        case AST_BOOL_LITERAL:
            printf("Bool %s\n", node->as.bool_value ? "true" : "false");
            break;
        case AST_STRING_LITERAL:
            printf("String \"%s\"\n", node->as.string_value);
            break;
        case AST_IDENTIFIER:
            printf("Identifier %s : %s\n", node->as.identifier, type_to_string(node->inferred_type));
            break;
        case AST_EMPTY:
            printf("Empty\n");
            break;
    }
}

void ast_free(AstNode *node) {
    if (!node) return;
    switch (node->kind) {
        case AST_PROGRAM:
            for (int i = 0; i < node->as.program.statements->count; i++) ast_free(node->as.program.statements->items[i]);
            free(node->as.program.statements->items);
            free(node->as.program.statements);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->as.block.statements->count; i++) ast_free(node->as.block.statements->items[i]);
            free(node->as.block.statements->items);
            free(node->as.block.statements);
            break;
        case AST_DECL:
            free(node->as.decl.name);
            ast_free(node->as.decl.init);
            break;
        case AST_ASSIGN:
            free(node->as.assign.name);
            ast_free(node->as.assign.value);
            break;
        case AST_IF:
            ast_free(node->as.if_stmt.condition);
            ast_free(node->as.if_stmt.then_branch);
            ast_free(node->as.if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->as.while_stmt.condition);
            ast_free(node->as.while_stmt.body);
            break;
        case AST_FOR:
            ast_free(node->as.for_stmt.init);
            ast_free(node->as.for_stmt.condition);
            ast_free(node->as.for_stmt.update);
            ast_free(node->as.for_stmt.body);
            break;
        case AST_SCAN:
            free(node->as.scan.name);
            break;
        case AST_PRINT:
            ast_free(node->as.print.value);
            break;
        case AST_BINARY:
            ast_free(node->as.binary.left);
            ast_free(node->as.binary.right);
            break;
        case AST_UNARY:
            ast_free(node->as.unary.operand);
            break;
        case AST_STRING_LITERAL:
            free(node->as.string_value);
            break;
        case AST_IDENTIFIER:
            free(node->as.identifier);
            break;
        default:
            break;
    }
    free(node);
}
