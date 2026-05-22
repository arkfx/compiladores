#include "codegen_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *c_type(TypeKind type) {
    return type == TYPE_BOOL ? "bool" : "int";
}

static void emit_indent(FILE *out) {
    fputs("    ", out);
}

static int already_declared(TacList *tac, int index, const char *target) {
    for (int i = 0; i < index; i++) {
        TacInstr *instr = &tac->items[i];
        if (instr->active && instr->kind == TAC_DECL && instr->target && strcmp(instr->target, target) == 0) {
            return 1;
        }
    }
    return 0;
}

static void emit_declarations(FILE *out, TacList *tac) {
    for (int i = 0; tac && i < tac->count; i++) {
        TacInstr *instr = &tac->items[i];
        if (!instr->active || instr->kind != TAC_DECL || !instr->target) continue;
        if (already_declared(tac, i, instr->target)) continue;
        emit_indent(out);
        fprintf(out, "%s %s = %s;\n",
                c_type(instr->value_type),
                instr->target,
                instr->value_type == TYPE_BOOL ? "false" : "0");
    }
}

static void emit_decl_uses(FILE *out, TacList *tac) {
    for (int i = 0; tac && i < tac->count; i++) {
        TacInstr *instr = &tac->items[i];
        if (!instr->active || instr->kind != TAC_DECL || !instr->target) continue;
        if (already_declared(tac, i, instr->target)) continue;
        emit_indent(out);
        fprintf(out, "(void)%s;\n", instr->target);
    }
}

static void emit_assign(FILE *out, TacInstr *instr) {
    emit_indent(out);
    fprintf(out, "%s = %s;\n", instr->target, instr->arg1);
}

static void emit_binary(FILE *out, TacInstr *instr) {
    emit_indent(out);
    fprintf(out, "%s = %s %s %s;\n", instr->target, instr->arg1, instr->op, instr->arg2);
}

static void emit_unary(FILE *out, TacInstr *instr) {
    emit_indent(out);
    fprintf(out, "%s = %s%s;\n", instr->target, instr->op, instr->arg1);
}

static void emit_scan(FILE *out, TacInstr *instr) {
    emit_indent(out);
    fputs("{\n", out);
    emit_indent(out);
    fputs("    int mc_scan_tmp = 0;\n", out);
    emit_indent(out);
    fputs("    scanf(\"%d\", &mc_scan_tmp);\n", out);
    emit_indent(out);
    if (instr->value_type == TYPE_BOOL) {
        fprintf(out, "    %s = mc_scan_tmp != 0;\n", instr->target);
    } else {
        fprintf(out, "    %s = mc_scan_tmp;\n", instr->target);
    }
    emit_indent(out);
    fputs("}\n", out);
}

static void emit_print(FILE *out, TacInstr *instr) {
    emit_indent(out);
    if (instr->value_type == TYPE_STRING_LITERAL) {
        fprintf(out, "printf(\"%%s\\n\", %s);\n", instr->arg1);
    } else if (instr->value_type == TYPE_BOOL) {
        fprintf(out, "printf(\"%%s\\n\", (%s) ? \"true\" : \"false\");\n", instr->arg1);
    } else {
        fprintf(out, "printf(\"%%d\\n\", %s);\n", instr->arg1);
    }
}

int codegen_c_file_from_tac(TacList *tac, const char *path) {
    FILE *out = fopen(path, "w");
    if (!out) {
        perror(path);
        return 1;
    }

    fputs("#include <stdio.h>\n#include <stdbool.h>\n\n", out);
    fputs("int main(void) {\n", out);
    emit_declarations(out, tac);

    for (int i = 0; tac && i < tac->count; i++) {
        TacInstr *instr = &tac->items[i];
        if (!instr->active) continue;
        switch (instr->kind) {
            case TAC_DECL:
                break;
            case TAC_ASSIGN:
                emit_assign(out, instr);
                break;
            case TAC_BINARY:
                emit_binary(out, instr);
                break;
            case TAC_UNARY:
                emit_unary(out, instr);
                break;
            case TAC_LABEL:
                fprintf(out, "%s:;\n", instr->label);
                break;
            case TAC_GOTO:
                emit_indent(out);
                fprintf(out, "goto %s;\n", instr->label);
                break;
            case TAC_IF_FALSE:
                emit_indent(out);
                fprintf(out, "if (!(%s)) goto %s;\n", instr->arg1, instr->label);
                break;
            case TAC_SCAN:
                emit_scan(out, instr);
                break;
            case TAC_PRINT:
                emit_print(out, instr);
                break;
        }
    }

    emit_decl_uses(out, tac);
    emit_indent(out);
    fputs("return 0;\n", out);
    fputs("}\n", out);
    fclose(out);
    return 0;
}
