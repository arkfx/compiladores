#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TypeKind analyze_node(SemanticContext *ctx, AstNode *node);

static char *xstrdup(const char *s) {
    char *copy = malloc(strlen(s) + 1);
    if (!copy) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(copy, s);
    return copy;
}

SemanticContext *semantic_context_new(void) {
    SemanticContext *ctx = calloc(1, sizeof(SemanticContext));
    if (!ctx) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    ctx->symbols = symbol_table_new();
    return ctx;
}

void semantic_context_free(SemanticContext *ctx) {
    if (!ctx) return;
    symbol_table_free(ctx->symbols);
    free(ctx);
}

static void sem_error(SemanticContext *ctx, int line, const char *fmt, ...) {
    va_list args;
    ctx->error_count++;
    fprintf(stderr, "semantic error at line %d: ", line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static int is_numeric_comparison(BinOp op) {
    return op == BIN_LT || op == BIN_LTE || op == BIN_GT || op == BIN_GTE;
}

static int is_equality(BinOp op) {
    return op == BIN_EQ || op == BIN_NEQ;
}

static int is_arithmetic(BinOp op) {
    return op == BIN_ADD || op == BIN_SUB || op == BIN_MUL || op == BIN_DIV || op == BIN_MOD;
}

static int is_logical(BinOp op) {
    return op == BIN_AND || op == BIN_OR;
}

static void require_bool_condition(SemanticContext *ctx, AstNode *condition, const char *owner) {
    TypeKind type = analyze_node(ctx, condition);
    if (type != TYPE_BOOL && type != TYPE_ERROR) {
        sem_error(ctx, condition->line, "%s condition must be bool, got %s", owner, type_to_string(type));
    }
}

static void analyze_statement_list(SemanticContext *ctx, AstList *list) {
    for (int i = 0; list && i < list->count; i++) {
        analyze_node(ctx, list->items[i]);
    }
}

static TypeKind analyze_node(SemanticContext *ctx, AstNode *node) {
    if (!node) return TYPE_VOID;

    switch (node->kind) {
        case AST_PROGRAM:
            analyze_statement_list(ctx, node->as.program.statements);
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;

        case AST_BLOCK:
            symbol_table_enter_scope(ctx->symbols);
            analyze_statement_list(ctx, node->as.block.statements);
            symbol_table_exit_scope(ctx->symbols);
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;

        case AST_DECL: {
            Symbol *sym = symbol_table_declare(ctx->symbols, node->as.decl.name, node->as.decl.type, node->line);
            if (!sym) {
                sem_error(ctx, node->line, "variable '%s' already declared in this scope", node->as.decl.name);
            } else {
                node->as.decl.c_name = xstrdup(sym->c_name);
                node->as.decl.address = sym->address;
            }
            if (node->as.decl.init) {
                TypeKind init_type = analyze_node(ctx, node->as.decl.init);
                if (init_type != node->as.decl.type && init_type != TYPE_ERROR) {
                    sem_error(ctx, node->line, "cannot initialize %s variable '%s' with %s expression",
                              type_to_string(node->as.decl.type),
                              node->as.decl.name,
                              type_to_string(init_type));
                }
            }
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_ASSIGN: {
            Symbol *sym = symbol_table_lookup(ctx->symbols, node->as.assign.name);
            TypeKind value_type = analyze_node(ctx, node->as.assign.value);
            if (!sym) {
                sem_error(ctx, node->line, "variable '%s' was not declared", node->as.assign.name);
                node->inferred_type = TYPE_ERROR;
                return TYPE_ERROR;
            }
            node->as.assign.c_name = xstrdup(sym->c_name);
            if (value_type != sym->type && value_type != TYPE_ERROR) {
                sem_error(ctx, node->line, "cannot assign %s expression to %s variable '%s'",
                          type_to_string(value_type),
                          type_to_string(sym->type),
                          node->as.assign.name);
            }
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_IF:
            require_bool_condition(ctx, node->as.if_stmt.condition, "if");
            analyze_node(ctx, node->as.if_stmt.then_branch);
            analyze_node(ctx, node->as.if_stmt.else_branch);
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;

        case AST_WHILE:
            require_bool_condition(ctx, node->as.while_stmt.condition, "while");
            analyze_node(ctx, node->as.while_stmt.body);
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;

        case AST_FOR:
            symbol_table_enter_scope(ctx->symbols);
            analyze_node(ctx, node->as.for_stmt.init);
            require_bool_condition(ctx, node->as.for_stmt.condition, "for");
            analyze_node(ctx, node->as.for_stmt.update);
            analyze_node(ctx, node->as.for_stmt.body);
            symbol_table_exit_scope(ctx->symbols);
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;

        case AST_SCAN: {
            Symbol *sym = symbol_table_lookup(ctx->symbols, node->as.scan.name);
            if (!sym) {
                sem_error(ctx, node->line, "variable '%s' was not declared", node->as.scan.name);
            } else if (sym->type != TYPE_INT && sym->type != TYPE_BOOL) {
                sem_error(ctx, node->line, "scan only supports int and bool variables");
            } else {
                node->as.scan.c_name = xstrdup(sym->c_name);
                node->as.scan.target_type = sym->type;
                node->inferred_type = sym->type;
            }
            return TYPE_VOID;
        }

        case AST_PRINT: {
            TypeKind type = analyze_node(ctx, node->as.print.value);
            if (type != TYPE_INT && type != TYPE_BOOL && type != TYPE_STRING_LITERAL && type != TYPE_ERROR) {
                sem_error(ctx, node->line, "print does not support %s values", type_to_string(type));
            }
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;
        }

        case AST_BINARY: {
            TypeKind left = analyze_node(ctx, node->as.binary.left);
            TypeKind right = analyze_node(ctx, node->as.binary.right);
            BinOp op = node->as.binary.op;
            if (left == TYPE_ERROR || right == TYPE_ERROR) {
                node->inferred_type = TYPE_ERROR;
                return TYPE_ERROR;
            }
            if (is_arithmetic(op)) {
                if (left != TYPE_INT || right != TYPE_INT) {
                    sem_error(ctx, node->line, "operator '%s' requires int operands", binop_to_string(op));
                    node->inferred_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                node->inferred_type = TYPE_INT;
                return TYPE_INT;
            }
            if (is_numeric_comparison(op)) {
                if (left != TYPE_INT || right != TYPE_INT) {
                    sem_error(ctx, node->line, "operator '%s' requires int operands", binop_to_string(op));
                    node->inferred_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                node->inferred_type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            if (is_equality(op)) {
                if (left != right || left == TYPE_STRING_LITERAL) {
                    sem_error(ctx, node->line, "operator '%s' requires compatible non-string operands", binop_to_string(op));
                    node->inferred_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                node->inferred_type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            if (is_logical(op)) {
                if (left != TYPE_BOOL || right != TYPE_BOOL) {
                    sem_error(ctx, node->line, "operator '%s' requires bool operands", binop_to_string(op));
                    node->inferred_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                node->inferred_type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            node->inferred_type = TYPE_ERROR;
            return TYPE_ERROR;
        }

        case AST_UNARY: {
            TypeKind operand = analyze_node(ctx, node->as.unary.operand);
            if (operand == TYPE_ERROR) {
                node->inferred_type = TYPE_ERROR;
                return TYPE_ERROR;
            }
            if (node->as.unary.op == UN_NEG) {
                if (operand != TYPE_INT) {
                    sem_error(ctx, node->line, "unary '-' requires int operand");
                    node->inferred_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                node->inferred_type = TYPE_INT;
                return TYPE_INT;
            }
            if (node->as.unary.op == UN_NOT) {
                if (operand != TYPE_BOOL) {
                    sem_error(ctx, node->line, "unary '!' requires bool operand");
                    node->inferred_type = TYPE_ERROR;
                    return TYPE_ERROR;
                }
                node->inferred_type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            node->inferred_type = TYPE_ERROR;
            return TYPE_ERROR;
        }

        case AST_INT_LITERAL:
            node->inferred_type = TYPE_INT;
            return TYPE_INT;

        case AST_BOOL_LITERAL:
            node->inferred_type = TYPE_BOOL;
            return TYPE_BOOL;

        case AST_STRING_LITERAL:
            node->inferred_type = TYPE_STRING_LITERAL;
            return TYPE_STRING_LITERAL;

        case AST_IDENTIFIER: {
            Symbol *sym = symbol_table_lookup(ctx->symbols, node->as.identifier.name);
            if (!sym) {
                sem_error(ctx, node->line, "variable '%s' was not declared", node->as.identifier.name);
                node->inferred_type = TYPE_ERROR;
                return TYPE_ERROR;
            }
            node->as.identifier.c_name = xstrdup(sym->c_name);
            node->inferred_type = sym->type;
            return sym->type;
        }

        case AST_EMPTY:
            node->inferred_type = TYPE_VOID;
            return TYPE_VOID;
    }

    node->inferred_type = TYPE_ERROR;
    return TYPE_ERROR;
}

int semantic_analyze(SemanticContext *ctx, AstNode *program) {
    ctx->error_count = 0;
    analyze_node(ctx, program);
    return ctx->error_count;
}
