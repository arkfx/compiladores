#include "codegen_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void emit_stmt(FILE *out, AstNode *node, int indent);
static char *emit_expr(AstNode *node);

static char *xstrdup(const char *s) {
    char *copy = malloc(strlen(s) + 1);
    if (!copy) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(copy, s);
    return copy;
}

static char *fmt(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int size = vsnprintf(NULL, 0, format, args);
    va_end(args);
    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    va_start(args, format);
    vsnprintf(buffer, (size_t)size + 1, format, args);
    va_end(args);
    return buffer;
}

static void write_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) fputs("    ", out);
}

static const char *c_type(TypeKind type) {
    return type == TYPE_BOOL ? "bool" : "int";
}

static char *escape_c_string(const char *s) {
    size_t extra = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '\\' || *p == '"' || *p == '\n' || *p == '\t') extra++;
    }
    char *out = malloc(strlen(s) + extra + 1);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    char *w = out;
    for (const char *p = s; *p; p++) {
        if (*p == '\\') { *w++ = '\\'; *w++ = '\\'; }
        else if (*p == '"') { *w++ = '\\'; *w++ = '"'; }
        else if (*p == '\n') { *w++ = '\\'; *w++ = 'n'; }
        else if (*p == '\t') { *w++ = '\\'; *w++ = 't'; }
        else { *w++ = *p; }
    }
    *w = '\0';
    return out;
}

int codegen_c_file(AstNode *program, const char *path) {
    FILE *out = fopen(path, "w");
    if (!out) {
        perror(path);
        return 1;
    }
    fputs("#include <stdio.h>\n#include <stdbool.h>\n\n", out);
    fputs("int main(void) {\n", out);
    emit_stmt(out, program, 1);
    fputs("    return 0;\n", out);
    fputs("}\n", out);
    fclose(out);
    return 0;
}

static void emit_list(FILE *out, AstList *list, int indent) {
    for (int i = 0; list && i < list->count; i++) {
        emit_stmt(out, list->items[i], indent);
    }
}

static void emit_inline_block(FILE *out, AstNode *block, int indent) {
    fputs("{\n", out);
    emit_list(out, block->as.block.statements, indent + 1);
    write_indent(out, indent);
    fputs("}\n", out);
}

static void emit_inline_assignment(FILE *out, AstNode *node) {
    char *value = emit_expr(node->as.assign.value);
    fprintf(out, "%s = %s", node->as.assign.name, value);
    free(value);
}

static void emit_inline_decl(FILE *out, AstNode *node) {
    fprintf(out, "%s %s", c_type(node->as.decl.type), node->as.decl.name);
    if (node->as.decl.init) {
        char *init = emit_expr(node->as.decl.init);
        fprintf(out, " = %s", init);
        free(init);
    }
}

static void emit_stmt(FILE *out, AstNode *node, int indent) {
    if (!node) return;
    switch (node->kind) {
        case AST_PROGRAM:
            emit_list(out, node->as.program.statements, indent);
            break;

        case AST_BLOCK:
            write_indent(out, indent);
            fputs("{\n", out);
            emit_list(out, node->as.block.statements, indent + 1);
            write_indent(out, indent);
            fputs("}\n", out);
            break;

        case AST_DECL:
            write_indent(out, indent);
            emit_inline_decl(out, node);
            fputs(";\n", out);
            break;

        case AST_ASSIGN:
            write_indent(out, indent);
            emit_inline_assignment(out, node);
            fputs(";\n", out);
            break;

        case AST_IF: {
            char *cond = emit_expr(node->as.if_stmt.condition);
            write_indent(out, indent);
            fprintf(out, "if (%s) ", cond);
            free(cond);
            if (node->as.if_stmt.then_branch && node->as.if_stmt.then_branch->kind == AST_BLOCK) {
                emit_inline_block(out, node->as.if_stmt.then_branch, indent);
            } else {
                fputs("{\n", out);
                emit_stmt(out, node->as.if_stmt.then_branch, indent + 1);
                write_indent(out, indent);
                fputs("}\n", out);
            }
            if (node->as.if_stmt.else_branch) {
                write_indent(out, indent);
                fputs("else ", out);
                if (node->as.if_stmt.else_branch->kind == AST_BLOCK) {
                    emit_inline_block(out, node->as.if_stmt.else_branch, indent);
                } else if (node->as.if_stmt.else_branch->kind == AST_IF) {
                    emit_stmt(out, node->as.if_stmt.else_branch, indent);
                } else {
                    fputs("{\n", out);
                    emit_stmt(out, node->as.if_stmt.else_branch, indent + 1);
                    write_indent(out, indent);
                    fputs("}\n", out);
                }
            }
            break;
        }

        case AST_WHILE: {
            char *cond = emit_expr(node->as.while_stmt.condition);
            write_indent(out, indent);
            fprintf(out, "while (%s) ", cond);
            free(cond);
            if (node->as.while_stmt.body && node->as.while_stmt.body->kind == AST_BLOCK) {
                emit_inline_block(out, node->as.while_stmt.body, indent);
            } else {
                fputs("{\n", out);
                emit_stmt(out, node->as.while_stmt.body, indent + 1);
                write_indent(out, indent);
                fputs("}\n", out);
            }
            break;
        }

        case AST_FOR: {
            char *cond = emit_expr(node->as.for_stmt.condition);
            write_indent(out, indent);
            fputs("for (", out);
            if (node->as.for_stmt.init->kind == AST_DECL) emit_inline_decl(out, node->as.for_stmt.init);
            else emit_inline_assignment(out, node->as.for_stmt.init);
            fprintf(out, "; %s; ", cond);
            emit_inline_assignment(out, node->as.for_stmt.update);
            fputs(") ", out);
            free(cond);
            if (node->as.for_stmt.body && node->as.for_stmt.body->kind == AST_BLOCK) {
                emit_inline_block(out, node->as.for_stmt.body, indent);
            } else {
                fputs("{\n", out);
                emit_stmt(out, node->as.for_stmt.body, indent + 1);
                write_indent(out, indent);
                fputs("}\n", out);
            }
            break;
        }

        case AST_SCAN:
            write_indent(out, indent);
            fputs("{\n", out);
            write_indent(out, indent + 1);
            fputs("int __scan_tmp = 0;\n", out);
            write_indent(out, indent + 1);
            fprintf(out, "scanf(\"%%d\", &__scan_tmp);\n");
            write_indent(out, indent + 1);
            if (node->inferred_type == TYPE_BOOL) {
                fprintf(out, "%s = __scan_tmp != 0", node->as.scan.name);
            } else {
                fprintf(out, "%s = __scan_tmp", node->as.scan.name);
            }
            fputs(";\n", out);
            write_indent(out, indent);
            fputs("}\n", out);
            break;

        case AST_PRINT: {
            AstNode *value = node->as.print.value;
            char *expr = emit_expr(value);
            write_indent(out, indent);
            if (value->inferred_type == TYPE_STRING_LITERAL) {
                fprintf(out, "printf(\"%%s\\n\", %s);\n", expr);
            } else if (value->inferred_type == TYPE_BOOL) {
                fprintf(out, "printf(\"%%s\\n\", (%s) ? \"true\" : \"false\");\n", expr);
            } else {
                fprintf(out, "printf(\"%%d\\n\", %s);\n", expr);
            }
            free(expr);
            break;
        }

        case AST_EMPTY:
            break;

        default:
            break;
    }
}

static char *emit_expr(AstNode *node) {
    if (!node) return xstrdup("0");
    switch (node->kind) {
        case AST_INT_LITERAL:
            return fmt("%d", node->as.int_value);
        case AST_BOOL_LITERAL:
            return xstrdup(node->as.bool_value ? "true" : "false");
        case AST_STRING_LITERAL: {
            char *escaped = escape_c_string(node->as.string_value);
            char *result = fmt("\"%s\"", escaped);
            free(escaped);
            return result;
        }
        case AST_IDENTIFIER:
            return xstrdup(node->as.identifier);
        case AST_UNARY: {
            char *operand = emit_expr(node->as.unary.operand);
            char *result = fmt("(%s%s)", unop_to_string(node->as.unary.op), operand);
            free(operand);
            return result;
        }
        case AST_BINARY: {
            char *left = emit_expr(node->as.binary.left);
            char *right = emit_expr(node->as.binary.right);
            char *result = fmt("(%s %s %s)", left, binop_to_string(node->as.binary.op), right);
            free(left);
            free(right);
            return result;
        }
        default:
            return xstrdup("0");
    }
}
