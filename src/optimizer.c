#include "optimizer.h"

static int is_int(AstNode *node, int value) {
    return node && node->kind == AST_INT_LITERAL && node->as.int_value == value;
}

static int is_bool_literal(AstNode *node) {
    return node && node->kind == AST_BOOL_LITERAL;
}

static int eval_int(BinOp op, int left, int right, int *ok) {
    *ok = 1;
    switch (op) {
        case BIN_ADD: return left + right;
        case BIN_SUB: return left - right;
        case BIN_MUL: return left * right;
        case BIN_DIV:
            if (right == 0) { *ok = 0; return 0; }
            return left / right;
        case BIN_MOD:
            if (right == 0) { *ok = 0; return 0; }
            return left % right;
        default:
            *ok = 0;
            return 0;
    }
}

static int eval_compare(BinOp op, int left, int right, int *ok) {
    *ok = 1;
    switch (op) {
        case BIN_EQ: return left == right;
        case BIN_NEQ: return left != right;
        case BIN_LT: return left < right;
        case BIN_LTE: return left <= right;
        case BIN_GT: return left > right;
        case BIN_GTE: return left >= right;
        default:
            *ok = 0;
            return 0;
    }
}

AstNode *optimize_ast(AstNode *node) {
    if (!node) return NULL;

    switch (node->kind) {
        case AST_PROGRAM:
            for (int i = 0; i < node->as.program.statements->count; i++) {
                node->as.program.statements->items[i] = optimize_ast(node->as.program.statements->items[i]);
            }
            return node;

        case AST_BLOCK:
            for (int i = 0; i < node->as.block.statements->count; i++) {
                node->as.block.statements->items[i] = optimize_ast(node->as.block.statements->items[i]);
            }
            return node;

        case AST_DECL:
            node->as.decl.init = optimize_ast(node->as.decl.init);
            return node;

        case AST_ASSIGN:
            node->as.assign.value = optimize_ast(node->as.assign.value);
            return node;

        case AST_IF:
            node->as.if_stmt.condition = optimize_ast(node->as.if_stmt.condition);
            node->as.if_stmt.then_branch = optimize_ast(node->as.if_stmt.then_branch);
            node->as.if_stmt.else_branch = optimize_ast(node->as.if_stmt.else_branch);
            if (is_bool_literal(node->as.if_stmt.condition)) {
                return node->as.if_stmt.condition->as.bool_value
                    ? node->as.if_stmt.then_branch
                    : (node->as.if_stmt.else_branch ? node->as.if_stmt.else_branch : ast_new_empty(node->line));
            }
            return node;

        case AST_WHILE:
            node->as.while_stmt.condition = optimize_ast(node->as.while_stmt.condition);
            node->as.while_stmt.body = optimize_ast(node->as.while_stmt.body);
            if (is_bool_literal(node->as.while_stmt.condition) && !node->as.while_stmt.condition->as.bool_value) {
                return ast_new_empty(node->line);
            }
            return node;

        case AST_FOR:
            node->as.for_stmt.init = optimize_ast(node->as.for_stmt.init);
            node->as.for_stmt.condition = optimize_ast(node->as.for_stmt.condition);
            node->as.for_stmt.update = optimize_ast(node->as.for_stmt.update);
            node->as.for_stmt.body = optimize_ast(node->as.for_stmt.body);
            if (is_bool_literal(node->as.for_stmt.condition) && !node->as.for_stmt.condition->as.bool_value) {
                return node->as.for_stmt.init;
            }
            return node;

        case AST_PRINT:
            node->as.print.value = optimize_ast(node->as.print.value);
            return node;

        case AST_BINARY: {
            node->as.binary.left = optimize_ast(node->as.binary.left);
            node->as.binary.right = optimize_ast(node->as.binary.right);
            AstNode *left = node->as.binary.left;
            AstNode *right = node->as.binary.right;
            BinOp op = node->as.binary.op;

            if (left && right && left->kind == AST_INT_LITERAL && right->kind == AST_INT_LITERAL) {
                int ok = 0;
                int result = eval_int(op, left->as.int_value, right->as.int_value, &ok);
                if (ok) return ast_new_int(result, node->line);
                result = eval_compare(op, left->as.int_value, right->as.int_value, &ok);
                if (ok) return ast_new_bool(result, node->line);
            }

            if (left && right && left->kind == AST_BOOL_LITERAL && right->kind == AST_BOOL_LITERAL) {
                if (op == BIN_AND) return ast_new_bool(left->as.bool_value && right->as.bool_value, node->line);
                if (op == BIN_OR) return ast_new_bool(left->as.bool_value || right->as.bool_value, node->line);
                if (op == BIN_EQ) return ast_new_bool(left->as.bool_value == right->as.bool_value, node->line);
                if (op == BIN_NEQ) return ast_new_bool(left->as.bool_value != right->as.bool_value, node->line);
            }

            if ((op == BIN_ADD || op == BIN_SUB) && is_int(right, 0)) return left;
            if (op == BIN_ADD && is_int(left, 0)) return right;
            if (op == BIN_MUL && is_int(right, 1)) return left;
            if (op == BIN_MUL && is_int(left, 1)) return right;
            if (op == BIN_MUL && (is_int(left, 0) || is_int(right, 0))) return ast_new_int(0, node->line);
            if (op == BIN_DIV && is_int(right, 1)) return left;
            return node;
        }

        case AST_UNARY:
            node->as.unary.operand = optimize_ast(node->as.unary.operand);
            if (node->as.unary.operand && node->as.unary.op == UN_NEG && node->as.unary.operand->kind == AST_INT_LITERAL) {
                return ast_new_int(-node->as.unary.operand->as.int_value, node->line);
            }
            if (node->as.unary.operand && node->as.unary.op == UN_NOT && node->as.unary.operand->kind == AST_BOOL_LITERAL) {
                return ast_new_bool(!node->as.unary.operand->as.bool_value, node->line);
            }
            return node;

        default:
            return node;
    }
}
