#include "tac.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *place;
    TypeKind type;
} ExprResult;

typedef struct {
    char *name;
    char *value;
    TypeKind type;
    int valid;
} ConstEntry;

typedef struct {
    ConstEntry *items;
    int count;
    int capacity;
} ConstTable;

static void gen_stmt(TacList *list, AstNode *node);
static ExprResult gen_expr(TacList *list, AstNode *node);

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

static int is_literal_place(const char *s) {
    if (!s || !*s) return 1;
    if (strcmp(s, "true") == 0 || strcmp(s, "false") == 0) return 1;
    if (*s == '"') return 1;
    if ((*s >= '0' && *s <= '9') || *s == '-') return 1;
    return 0;
}

static int is_temp_name(const char *s) {
    return s && strncmp(s, "mc_t", 4) == 0;
}

static int is_control_kind(TacKind kind) {
    return kind == TAC_LABEL || kind == TAC_GOTO || kind == TAC_IF_FALSE;
}

static char *reads1(const char *a) {
    return is_literal_place(a) ? xstrdup("") : xstrdup(a);
}

static char *reads2(const char *a, const char *b) {
    int alit = is_literal_place(a);
    int blit = is_literal_place(b);
    if (alit && blit) return xstrdup("");
    if (alit) return xstrdup(b);
    if (blit) return xstrdup(a);
    return fmt("%s,%s", a, b);
}

static void free_instr_fields(TacInstr *instr) {
    free(instr->text);
    free(instr->target);
    free(instr->arg1);
    free(instr->arg2);
    free(instr->op);
    free(instr->label);
    free(instr->reads);
    free(instr->writes);
    memset(instr, 0, sizeof(TacInstr));
}

static char *render_instr(TacInstr *instr) {
    switch (instr->kind) {
        case TAC_DECL:
            return fmt("decl %s %s ; addr %d", type_to_string(instr->value_type), instr->target, instr->address);
        case TAC_ASSIGN:
            return fmt("%s = %s", instr->target, instr->arg1);
        case TAC_BINARY:
            return fmt("%s = %s %s %s", instr->target, instr->arg1, instr->op, instr->arg2);
        case TAC_UNARY:
            return fmt("%s = %s%s", instr->target, instr->op, instr->arg1);
        case TAC_LABEL:
            return fmt("%s:", instr->label);
        case TAC_GOTO:
            return fmt("goto %s", instr->label);
        case TAC_IF_FALSE:
            return fmt("ifFalse %s goto %s", instr->arg1, instr->label);
        case TAC_SCAN:
            return fmt("scan %s", instr->target);
        case TAC_PRINT:
            return fmt("print %s", instr->arg1);
    }
    return xstrdup("<invalid>");
}

static void refresh_instr(TacInstr *instr) {
    free(instr->text);
    instr->text = render_instr(instr);
    free(instr->reads);
    free(instr->writes);
    instr->reads = NULL;
    instr->writes = NULL;
    switch (instr->kind) {
        case TAC_DECL:
            instr->reads = xstrdup("");
            instr->writes = xstrdup(instr->target);
            break;
        case TAC_ASSIGN:
            instr->reads = reads1(instr->arg1);
            instr->writes = xstrdup(instr->target);
            break;
        case TAC_BINARY:
            instr->reads = reads2(instr->arg1, instr->arg2);
            instr->writes = xstrdup(instr->target);
            break;
        case TAC_UNARY:
            instr->reads = reads1(instr->arg1);
            instr->writes = xstrdup(instr->target);
            break;
        case TAC_IF_FALSE:
        case TAC_PRINT:
            instr->reads = reads1(instr->arg1);
            instr->writes = xstrdup("");
            break;
        case TAC_SCAN:
            instr->reads = xstrdup("");
            instr->writes = xstrdup(instr->target);
            break;
        default:
            instr->reads = xstrdup("");
            instr->writes = xstrdup("");
            break;
    }
}

static TacInstr *tac_add(TacList *list, TacKind kind) {
    if (list->count == list->capacity) {
        list->capacity *= 2;
        list->items = realloc(list->items, (size_t)list->capacity * sizeof(TacInstr));
        if (!list->items) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
    }
    TacInstr *instr = &list->items[list->count++];
    memset(instr, 0, sizeof(TacInstr));
    instr->kind = kind;
    instr->value_type = TYPE_VOID;
    instr->address = -1;
    instr->active = 1;
    return instr;
}

static void finish_instr(TacInstr *instr) {
    refresh_instr(instr);
}

static char *new_label(TacList *list) {
    return fmt("L%d", list->label_count++);
}

static ExprResult expr_result(char *place, TypeKind type) {
    ExprResult result;
    result.place = place;
    result.type = type;
    return result;
}

static ExprResult new_temp(TacList *list, TypeKind type) {
    char *name = fmt("mc_t%d", list->temp_count++);
    TacInstr *decl = tac_add(list, TAC_DECL);
    decl->target = xstrdup(name);
    decl->value_type = type;
    finish_instr(decl);
    return expr_result(name, type);
}

TacList *tac_generate(AstNode *program) {
    TacList *list = calloc(1, sizeof(TacList));
    if (!list) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    list->capacity = 64;
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

static void add_decl(TacList *list, AstNode *node) {
    TacInstr *decl = tac_add(list, TAC_DECL);
    decl->target = xstrdup(node->as.decl.c_name ? node->as.decl.c_name : node->as.decl.name);
    decl->value_type = node->as.decl.type;
    decl->address = node->as.decl.address;
    finish_instr(decl);
}

static void add_assign(TacList *list, const char *target, ExprResult value) {
    TacInstr *assign = tac_add(list, TAC_ASSIGN);
    assign->target = xstrdup(target);
    assign->arg1 = value.place;
    assign->value_type = value.type;
    finish_instr(assign);
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
            add_decl(list, node);
            if (node->as.decl.init) {
                ExprResult value = gen_expr(list, node->as.decl.init);
                add_assign(list, node->as.decl.c_name ? node->as.decl.c_name : node->as.decl.name, value);
            }
            break;
        case AST_ASSIGN: {
            ExprResult value = gen_expr(list, node->as.assign.value);
            add_assign(list, node->as.assign.c_name ? node->as.assign.c_name : node->as.assign.name, value);
            break;
        }
        case AST_IF: {
            char *else_label = new_label(list);
            char *end_label = new_label(list);
            ExprResult cond = gen_expr(list, node->as.if_stmt.condition);
            TacInstr *jump = tac_add(list, TAC_IF_FALSE);
            jump->arg1 = cond.place;
            jump->label = xstrdup(else_label);
            finish_instr(jump);
            gen_stmt(list, node->as.if_stmt.then_branch);
            if (node->as.if_stmt.else_branch) {
                TacInstr *go_end = tac_add(list, TAC_GOTO);
                go_end->label = xstrdup(end_label);
                finish_instr(go_end);
                TacInstr *else_instr = tac_add(list, TAC_LABEL);
                else_instr->label = xstrdup(else_label);
                finish_instr(else_instr);
                gen_stmt(list, node->as.if_stmt.else_branch);
                TacInstr *end_instr = tac_add(list, TAC_LABEL);
                end_instr->label = xstrdup(end_label);
                finish_instr(end_instr);
            } else {
                TacInstr *else_instr = tac_add(list, TAC_LABEL);
                else_instr->label = xstrdup(else_label);
                finish_instr(else_instr);
            }
            free(else_label);
            free(end_label);
            break;
        }
        case AST_WHILE: {
            char *start = new_label(list);
            char *end = new_label(list);
            TacInstr *start_instr = tac_add(list, TAC_LABEL);
            start_instr->label = xstrdup(start);
            finish_instr(start_instr);
            ExprResult cond = gen_expr(list, node->as.while_stmt.condition);
            TacInstr *jump = tac_add(list, TAC_IF_FALSE);
            jump->arg1 = cond.place;
            jump->label = xstrdup(end);
            finish_instr(jump);
            gen_stmt(list, node->as.while_stmt.body);
            TacInstr *go_start = tac_add(list, TAC_GOTO);
            go_start->label = xstrdup(start);
            finish_instr(go_start);
            TacInstr *end_instr = tac_add(list, TAC_LABEL);
            end_instr->label = xstrdup(end);
            finish_instr(end_instr);
            free(start);
            free(end);
            break;
        }
        case AST_FOR: {
            char *start = new_label(list);
            char *end = new_label(list);
            gen_stmt(list, node->as.for_stmt.init);
            TacInstr *start_instr = tac_add(list, TAC_LABEL);
            start_instr->label = xstrdup(start);
            finish_instr(start_instr);
            ExprResult cond = gen_expr(list, node->as.for_stmt.condition);
            TacInstr *jump = tac_add(list, TAC_IF_FALSE);
            jump->arg1 = cond.place;
            jump->label = xstrdup(end);
            finish_instr(jump);
            gen_stmt(list, node->as.for_stmt.body);
            gen_stmt(list, node->as.for_stmt.update);
            TacInstr *go_start = tac_add(list, TAC_GOTO);
            go_start->label = xstrdup(start);
            finish_instr(go_start);
            TacInstr *end_instr = tac_add(list, TAC_LABEL);
            end_instr->label = xstrdup(end);
            finish_instr(end_instr);
            free(start);
            free(end);
            break;
        }
        case AST_SCAN: {
            TacInstr *scan = tac_add(list, TAC_SCAN);
            scan->target = xstrdup(node->as.scan.c_name ? node->as.scan.c_name : node->as.scan.name);
            scan->value_type = node->as.scan.target_type;
            finish_instr(scan);
            break;
        }
        case AST_PRINT: {
            ExprResult value = gen_expr(list, node->as.print.value);
            TacInstr *print = tac_add(list, TAC_PRINT);
            print->arg1 = value.place;
            print->value_type = value.type;
            finish_instr(print);
            break;
        }
        case AST_EMPTY:
            break;
        default:
            break;
    }
}

static ExprResult gen_expr(TacList *list, AstNode *node) {
    if (!node) return expr_result(xstrdup("0"), TYPE_INT);
    switch (node->kind) {
        case AST_INT_LITERAL:
            return expr_result(fmt("%d", node->as.int_value), TYPE_INT);
        case AST_BOOL_LITERAL:
            return expr_result(xstrdup(node->as.bool_value ? "true" : "false"), TYPE_BOOL);
        case AST_STRING_LITERAL: {
            char *escaped = escape_c_string(node->as.string_value);
            char *literal = fmt("\"%s\"", escaped);
            free(escaped);
            return expr_result(literal, TYPE_STRING_LITERAL);
        }
        case AST_IDENTIFIER:
            return expr_result(xstrdup(node->as.identifier.c_name ? node->as.identifier.c_name : node->as.identifier.name),
                               node->inferred_type);
        case AST_UNARY: {
            ExprResult operand = gen_expr(list, node->as.unary.operand);
            ExprResult temp = new_temp(list, node->inferred_type);
            TacInstr *instr = tac_add(list, TAC_UNARY);
            instr->target = xstrdup(temp.place);
            instr->arg1 = operand.place;
            instr->op = xstrdup(unop_to_string(node->as.unary.op));
            instr->value_type = node->inferred_type;
            finish_instr(instr);
            return temp;
        }
        case AST_BINARY: {
            ExprResult left = gen_expr(list, node->as.binary.left);
            ExprResult right = gen_expr(list, node->as.binary.right);
            ExprResult temp = new_temp(list, node->inferred_type);
            TacInstr *instr = tac_add(list, TAC_BINARY);
            instr->target = xstrdup(temp.place);
            instr->arg1 = left.place;
            instr->arg2 = right.place;
            instr->op = xstrdup(binop_to_string(node->as.binary.op));
            instr->value_type = node->inferred_type;
            finish_instr(instr);
            return temp;
        }
        default:
            return expr_result(xstrdup("0"), TYPE_INT);
    }
}

static void const_table_free(ConstTable *table) {
    for (int i = 0; i < table->count; i++) {
        free(table->items[i].name);
        free(table->items[i].value);
    }
    free(table->items);
    memset(table, 0, sizeof(ConstTable));
}

static ConstEntry *const_find(ConstTable *table, const char *name) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->items[i].name, name) == 0) return &table->items[i];
    }
    return NULL;
}

static void const_clear(ConstTable *table) {
    for (int i = 0; i < table->count; i++) table->items[i].valid = 0;
}

static void const_set(ConstTable *table, const char *name, const char *value, TypeKind type) {
    if (!name || !*name || is_literal_place(name)) return;
    ConstEntry *entry = const_find(table, name);
    if (!entry) {
        if (table->count == table->capacity) {
            table->capacity = table->capacity ? table->capacity * 2 : 16;
            table->items = realloc(table->items, (size_t)table->capacity * sizeof(ConstEntry));
            if (!table->items) {
                fprintf(stderr, "out of memory\n");
                exit(1);
            }
        }
        entry = &table->items[table->count++];
        entry->name = xstrdup(name);
        entry->value = NULL;
    }
    free(entry->value);
    entry->value = xstrdup(value);
    entry->type = type;
    entry->valid = 1;
}

static void const_invalidate(ConstTable *table, const char *name) {
    ConstEntry *entry = const_find(table, name);
    if (entry) entry->valid = 0;
}

static const char *const_value(ConstTable *table, const char *name) {
    ConstEntry *entry = const_find(table, name);
    return entry && entry->valid ? entry->value : NULL;
}

static void replace_with_const(ConstTable *table, char **place) {
    const char *value = const_value(table, *place);
    if (value) {
        free(*place);
        *place = xstrdup(value);
    }
}

static int literal_int(const char *s, int *value) {
    if (!s || !*s || *s == '"') return 0;
    char *end = NULL;
    long parsed = strtol(s, &end, 10);
    if (!end || *end != '\0') return 0;
    *value = (int)parsed;
    return 1;
}

static int literal_bool(const char *s, int *value) {
    if (strcmp(s, "true") == 0) {
        *value = 1;
        return 1;
    }
    if (strcmp(s, "false") == 0) {
        *value = 0;
        return 1;
    }
    return 0;
}

static int eval_int_op(const char *op, int a, int b, int *ok) {
    *ok = 1;
    if (strcmp(op, "+") == 0) return a + b;
    if (strcmp(op, "-") == 0) return a - b;
    if (strcmp(op, "*") == 0) return a * b;
    if (strcmp(op, "/") == 0) {
        if (b == 0) { *ok = 0; return 0; }
        return a / b;
    }
    if (strcmp(op, "%") == 0) {
        if (b == 0) { *ok = 0; return 0; }
        return a % b;
    }
    if (strcmp(op, "==") == 0) return a == b;
    if (strcmp(op, "!=") == 0) return a != b;
    if (strcmp(op, "<") == 0) return a < b;
    if (strcmp(op, "<=") == 0) return a <= b;
    if (strcmp(op, ">") == 0) return a > b;
    if (strcmp(op, ">=") == 0) return a >= b;
    *ok = 0;
    return 0;
}

static int eval_bool_op(const char *op, int a, int b, int *ok) {
    *ok = 1;
    if (strcmp(op, "&&") == 0) return a && b;
    if (strcmp(op, "||") == 0) return a || b;
    if (strcmp(op, "==") == 0) return a == b;
    if (strcmp(op, "!=") == 0) return a != b;
    *ok = 0;
    return 0;
}

static void make_assign_literal(TacInstr *instr, const char *literal) {
    free(instr->arg1);
    free(instr->arg2);
    free(instr->op);
    instr->kind = TAC_ASSIGN;
    instr->arg1 = xstrdup(literal);
    instr->arg2 = NULL;
    instr->op = NULL;
    refresh_instr(instr);
}

static void propagate_constants(TacList *list) {
    ConstTable table = {0};
    for (int i = 0; i < list->count; i++) {
        TacInstr *instr = &list->items[i];
        if (!instr->active) continue;

        if (is_control_kind(instr->kind)) {
            if (instr->kind == TAC_IF_FALSE) replace_with_const(&table, &instr->arg1);
            const_clear(&table);
            refresh_instr(instr);
            continue;
        }

        if (instr->kind == TAC_SCAN) {
            const_invalidate(&table, instr->target);
            continue;
        }

        if (instr->kind == TAC_ASSIGN) {
            replace_with_const(&table, &instr->arg1);
            if (is_literal_place(instr->arg1)) const_set(&table, instr->target, instr->arg1, instr->value_type);
            else const_invalidate(&table, instr->target);
            refresh_instr(instr);
            continue;
        }

        if (instr->kind == TAC_UNARY) {
            replace_with_const(&table, &instr->arg1);
            int value = 0;
            if (strcmp(instr->op, "-") == 0 && literal_int(instr->arg1, &value)) {
                char *literal = fmt("%d", -value);
                make_assign_literal(instr, literal);
                const_set(&table, instr->target, literal, instr->value_type);
                free(literal);
            } else if (strcmp(instr->op, "!") == 0 && literal_bool(instr->arg1, &value)) {
                make_assign_literal(instr, value ? "false" : "true");
                const_set(&table, instr->target, value ? "false" : "true", instr->value_type);
            } else {
                const_invalidate(&table, instr->target);
                refresh_instr(instr);
            }
            continue;
        }

        if (instr->kind == TAC_BINARY) {
            replace_with_const(&table, &instr->arg1);
            replace_with_const(&table, &instr->arg2);
            int a = 0, b = 0, ok = 0;
            if (literal_int(instr->arg1, &a) && literal_int(instr->arg2, &b)) {
                int result = eval_int_op(instr->op, a, b, &ok);
                if (ok) {
                    char *literal = instr->value_type == TYPE_BOOL
                        ? xstrdup(result ? "true" : "false")
                        : fmt("%d", result);
                    make_assign_literal(instr, literal);
                    const_set(&table, instr->target, literal, instr->value_type);
                    free(literal);
                    continue;
                }
            }
            if (literal_bool(instr->arg1, &a) && literal_bool(instr->arg2, &b)) {
                int result = eval_bool_op(instr->op, a, b, &ok);
                if (ok) {
                    make_assign_literal(instr, result ? "true" : "false");
                    const_set(&table, instr->target, result ? "true" : "false", instr->value_type);
                    continue;
                }
            }
            const_invalidate(&table, instr->target);
            refresh_instr(instr);
        }
    }
    const_table_free(&table);
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

static int name_referenced(TacList *list, const char *name) {
    for (int i = 0; i < list->count; i++) {
        TacInstr *instr = &list->items[i];
        if (!instr->active) continue;
        if (contains_name(instr->reads, name)) return 1;
        if (instr->kind == TAC_PRINT && instr->arg1 && strcmp(instr->arg1, name) == 0) return 1;
        if (instr->kind == TAC_IF_FALSE && instr->arg1 && strcmp(instr->arg1, name) == 0) return 1;
    }
    return 0;
}

static int name_written_or_read_non_decl(TacList *list, const char *name) {
    for (int i = 0; i < list->count; i++) {
        TacInstr *instr = &list->items[i];
        if (!instr->active || instr->kind == TAC_DECL) continue;
        if ((instr->target && strcmp(instr->target, name) == 0) || contains_name(instr->reads, name)) return 1;
    }
    return 0;
}

static void remove_dead_temps(TacList *list) {
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i = 0; i < list->count; i++) {
            TacInstr *instr = &list->items[i];
            if (!instr->active || !instr->target || !is_temp_name(instr->target)) continue;
            if (instr->kind == TAC_ASSIGN || instr->kind == TAC_BINARY || instr->kind == TAC_UNARY) {
                if (!name_referenced(list, instr->target)) {
                    instr->active = 0;
                    changed = 1;
                }
            }
        }
    }

    for (int i = 0; i < list->count; i++) {
        TacInstr *instr = &list->items[i];
        if (instr->active && instr->kind == TAC_DECL && is_temp_name(instr->target)
            && !name_written_or_read_non_decl(list, instr->target)) {
            instr->active = 0;
        }
    }
}

static void remove_unreachable_after_goto(TacList *list) {
    int unreachable = 0;
    for (int i = 0; i < list->count; i++) {
        TacInstr *instr = &list->items[i];
        if (!instr->active) continue;
        if (unreachable && instr->kind != TAC_LABEL) {
            instr->active = 0;
            continue;
        }
        if (instr->kind == TAC_LABEL) {
            unreachable = 0;
            continue;
        }
        if (instr->kind == TAC_GOTO) unreachable = 1;
    }
}

void tac_optimize(TacList *list) {
    propagate_constants(list);
    remove_unreachable_after_goto(list);
    remove_dead_temps(list);
}

void tac_print(TacList *list) {
    int printed = 1;
    for (int i = 0; list && i < list->count; i++) {
        if (!list->items[i].active) continue;
        printf("%03d: %s\n", printed++, list->items[i].text);
    }
}

static int dependent(TacInstr *a, TacInstr *b) {
    return contains_name(b->reads, a->writes)
        || contains_name(a->reads, b->writes)
        || contains_name(a->writes, b->writes);
}

void tac_analyze_dependencies(TacList *list) {
    tac_print(list);
    printf("\nDependency analysis:\n");
    int visible_i = 0;
    for (int i = 0; list && i < list->count; i++) {
        if (!list->items[i].active) continue;
        visible_i++;
        int visible_j = visible_i;
        for (int j = i + 1; j < list->count && visible_j < visible_i + 4; j++) {
            if (!list->items[j].active) continue;
            visible_j++;
            if (!*list->items[i].writes && !*list->items[j].writes) continue;
            printf("  %03d and %03d: %s\n",
                   visible_i,
                   visible_j,
                   dependent(&list->items[i], &list->items[j]) ? "dependent" : "independent");
        }
    }
}

void tac_free(TacList *list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free_instr_fields(&list->items[i]);
    }
    free(list->items);
    free(list);
}
