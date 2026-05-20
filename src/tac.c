#include "tac.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void gen_stmt(TacList *list, AstNode *node);
static char *gen_expr(TacList *list, AstNode *node);

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

static void tac_add(TacList *list, char *text, const char *reads, const char *writes) {
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, (size_t)list->capacity * sizeof(TacInstr));
        if (!list->items) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    list->items[list->count].text = text;
    list->items[list->count].reads = xstrdup(reads ? reads : "");
    list->items[list->count].writes = xstrdup(writes ? writes : "");
    list->count++;
}

static char *new_temp(TacList *list) {
    return fmt("t%d", list->temp_count++);
}

static char *new_label(TacList *list) {
    return fmt("L%d", list->label_count++);
}

static int is_literal_place(const char *s) {
    if (!s || !*s) return 1;
    if (strcmp(s, "true") == 0 || strcmp(s, "false") == 0) return 1;
    if (*s == '"') return 1;
    if ((*s >= '0' && *s <= '9') || *s == '-') return 1;
    return 0;
}

static char *reads2(const char *a, const char *b) {
    int alit = is_literal_place(a);
    int blit = is_literal_place(b);
    if (alit && blit) return xstrdup("");
    if (alit) return xstrdup(b);
    if (blit) return xstrdup(a);
    return fmt("%s,%s", a, b);
}

TacList *tac_generate(AstNode *program) {
    TacList *list = calloc(1, sizeof(TacList));
    if (!list) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    list->capacity = 32;
    list->items = calloc((size_t)list->capacity, sizeof(TacInstr));
    if (!list->items) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    gen_stmt(list, program);
    return list;
}

static void gen_list(TacList *list, AstList *stmts) {
    for (int i = 0; stmts && i < stmts->count; i++) {
        gen_stmt(list, stmts->items[i]);
    }
}

static void gen_stmt(TacList *list, AstNode *node) {
    if (!node) return;
    switch (node->kind) {
        case AST_PROGRAM:
            gen_list(list, node->as.program.statements);
            break;
        case AST_BLOCK:
            gen_list(list, node->as.block.statements);
            break;
        case AST_DECL:
            tac_add(list, fmt("decl %s %s", type_to_string(node->as.decl.type), node->as.decl.name), "", node->as.decl.name);
            if (node->as.decl.init) {
                char *value = gen_expr(list, node->as.decl.init);
                tac_add(list, fmt("%s = %s", node->as.decl.name, value), is_literal_place(value) ? "" : value, node->as.decl.name);
                free(value);
            }
            break;
        case AST_ASSIGN: {
            char *value = gen_expr(list, node->as.assign.value);
            tac_add(list, fmt("%s = %s", node->as.assign.name, value), is_literal_place(value) ? "" : value, node->as.assign.name);
            free(value);
            break;
        }
        case AST_IF: {
            char *else_label = new_label(list);
            char *end_label = new_label(list);
            char *cond = gen_expr(list, node->as.if_stmt.condition);
            tac_add(list, fmt("ifFalse %s goto %s", cond, else_label), is_literal_place(cond) ? "" : cond, "");
            gen_stmt(list, node->as.if_stmt.then_branch);
            if (node->as.if_stmt.else_branch) {
                tac_add(list, fmt("goto %s", end_label), "", "");
                tac_add(list, fmt("%s:", else_label), "", "");
                gen_stmt(list, node->as.if_stmt.else_branch);
                tac_add(list, fmt("%s:", end_label), "", "");
            } else {
                tac_add(list, fmt("%s:", else_label), "", "");
            }
            free(cond);
            free(else_label);
            free(end_label);
            break;
        }
        case AST_WHILE: {
            char *start = new_label(list);
            char *end = new_label(list);
            tac_add(list, fmt("%s:", start), "", "");
            char *cond = gen_expr(list, node->as.while_stmt.condition);
            tac_add(list, fmt("ifFalse %s goto %s", cond, end), is_literal_place(cond) ? "" : cond, "");
            gen_stmt(list, node->as.while_stmt.body);
            tac_add(list, fmt("goto %s", start), "", "");
            tac_add(list, fmt("%s:", end), "", "");
            free(cond);
            free(start);
            free(end);
            break;
        }
        case AST_FOR: {
            char *start = new_label(list);
            char *end = new_label(list);
            gen_stmt(list, node->as.for_stmt.init);
            tac_add(list, fmt("%s:", start), "", "");
            char *cond = gen_expr(list, node->as.for_stmt.condition);
            tac_add(list, fmt("ifFalse %s goto %s", cond, end), is_literal_place(cond) ? "" : cond, "");
            gen_stmt(list, node->as.for_stmt.body);
            gen_stmt(list, node->as.for_stmt.update);
            tac_add(list, fmt("goto %s", start), "", "");
            tac_add(list, fmt("%s:", end), "", "");
            free(cond);
            free(start);
            free(end);
            break;
        }
        case AST_SCAN:
            tac_add(list, fmt("scan %s", node->as.scan.name), "", node->as.scan.name);
            break;
        case AST_PRINT: {
            char *value = gen_expr(list, node->as.print.value);
            tac_add(list, fmt("print %s", value), is_literal_place(value) ? "" : value, "");
            free(value);
            break;
        }
        case AST_EMPTY:
            break;
        default:
            break;
    }
}

static char *gen_expr(TacList *list, AstNode *node) {
    if (!node) return xstrdup("");
    switch (node->kind) {
        case AST_INT_LITERAL:
            return fmt("%d", node->as.int_value);
        case AST_BOOL_LITERAL:
            return xstrdup(node->as.bool_value ? "true" : "false");
        case AST_STRING_LITERAL:
            return fmt("\"%s\"", node->as.string_value);
        case AST_IDENTIFIER:
            return xstrdup(node->as.identifier);
        case AST_UNARY: {
            char *operand = gen_expr(list, node->as.unary.operand);
            char *temp = new_temp(list);
            tac_add(list, fmt("%s = %s%s", temp, unop_to_string(node->as.unary.op), operand),
                    is_literal_place(operand) ? "" : operand,
                    temp);
            free(operand);
            return temp;
        }
        case AST_BINARY: {
            char *left = gen_expr(list, node->as.binary.left);
            char *right = gen_expr(list, node->as.binary.right);
            char *temp = new_temp(list);
            char *reads = reads2(left, right);
            tac_add(list, fmt("%s = %s %s %s", temp, left, binop_to_string(node->as.binary.op), right), reads, temp);
            free(left);
            free(right);
            free(reads);
            return temp;
        }
        default:
            return xstrdup("<stmt>");
    }
}

void tac_print(TacList *list) {
    for (int i = 0; list && i < list->count; i++) {
        printf("%03d: %s\n", i + 1, list->items[i].text);
    }
}

static int contains_name(const char *csv, const char *name) {
    if (!csv || !name || !*csv || !*name) return 0;
    const char *p = csv;
    size_t n = strlen(name);
    while (*p) {
        while (*p == ',') p++;
        if (strncmp(p, name, n) == 0 && (p[n] == '\0' || p[n] == ',')) return 1;
        while (*p && *p != ',') p++;
    }
    return 0;
}

static int dependent(TacInstr *a, TacInstr *b) {
    return contains_name(b->reads, a->writes)
        || contains_name(a->reads, b->writes)
        || contains_name(a->writes, b->writes);
}

void tac_analyze_dependencies(TacList *list) {
    tac_print(list);
    printf("\nDependency analysis:\n");
    for (int i = 0; list && i < list->count; i++) {
        for (int j = i + 1; j < list->count && j < i + 4; j++) {
            if (!*list->items[i].writes && !*list->items[j].writes) continue;
            printf("  %03d and %03d: %s\n",
                   i + 1,
                   j + 1,
                   dependent(&list->items[i], &list->items[j]) ? "dependent" : "independent");
        }
    }
}

void tac_free(TacList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free(list->items[i].text);
        free(list->items[i].reads);
        free(list->items[i].writes);
    }
    free(list->items);
    free(list);
}
