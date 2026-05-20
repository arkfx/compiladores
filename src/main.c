#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "codegen_c.h"
#include "optimizer.h"
#include "semantic.h"
#include "tac.h"
#include "parser.tab.h"

extern FILE *yyin;
extern int yyparse(void);
extern int yylex(void);
extern AstNode *parsed_program;
extern YYLTYPE yylloc;

typedef enum {
    MODE_CODEGEN,
    MODE_TOKENS,
    MODE_AST,
    MODE_SYMBOLS,
    MODE_TAC,
    MODE_OPT_TAC,
    MODE_ANALYZE,
    MODE_COMPILE
} CompileMode;

typedef struct {
    CompileMode mode;
    const char *input_path;
    const char *output_path;
} CompileOptions;

static void usage(const char *prog) {
    fprintf(stderr,
            "usage: %s [--tokens|--ast|--symbols|--tac|--opt-tac|--analyze|--compile] input.ml [-o output.c]\n",
            prog);
}

static const char *token_name(int token) {
    switch (token) {
        case INT_LITERAL: return "INT_LITERAL";
        case BOOL_LITERAL: return "BOOL_LITERAL";
        case IDENTIFIER: return "IDENTIFIER";
        case STRING_LITERAL: return "STRING_LITERAL";
        case KW_INT: return "KW_INT";
        case KW_BOOL: return "KW_BOOL";
        case KW_IF: return "KW_IF";
        case KW_ELSE: return "KW_ELSE";
        case KW_WHILE: return "KW_WHILE";
        case KW_FOR: return "KW_FOR";
        case KW_SCAN: return "KW_SCAN";
        case KW_PRINT: return "KW_PRINT";
        case EQ: return "EQ";
        case NEQ: return "NEQ";
        case LTE: return "LTE";
        case GTE: return "GTE";
        case AND: return "AND";
        case OR: return "OR";
        default: break;
    }
    static char buf[32];
    if (token > 0 && token < 128) {
        snprintf(buf, sizeof(buf), "'%c'", token);
    } else {
        snprintf(buf, sizeof(buf), "TOKEN_%d", token);
    }
    return buf;
}

static int parse_args(int argc, char **argv, CompileOptions *opts) {
    opts->mode = MODE_CODEGEN;
    opts->input_path = NULL;
    opts->output_path = "out.c";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tokens") == 0) opts->mode = MODE_TOKENS;
        else if (strcmp(argv[i], "--ast") == 0) opts->mode = MODE_AST;
        else if (strcmp(argv[i], "--symbols") == 0) opts->mode = MODE_SYMBOLS;
        else if (strcmp(argv[i], "--tac") == 0) opts->mode = MODE_TAC;
        else if (strcmp(argv[i], "--opt-tac") == 0) opts->mode = MODE_OPT_TAC;
        else if (strcmp(argv[i], "--analyze") == 0) opts->mode = MODE_ANALYZE;
        else if (strcmp(argv[i], "--compile") == 0) opts->mode = MODE_COMPILE;
        else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) return 1;
            opts->output_path = argv[++i];
        } else if (argv[i][0] == '-') {
            return 1;
        } else {
            opts->input_path = argv[i];
        }
    }

    return opts->input_path == NULL;
}

static int print_tokens(const char *path) {
    yyin = fopen(path, "r");
    if (!yyin) {
        perror(path);
        return 1;
    }
    int token;
    while ((token = yylex()) != 0) {
        printf("%4d:%-3d %-16s", yylloc.first_line, yylloc.first_column, token_name(token));
        if (token == INT_LITERAL) printf(" %d", yylval.int_value);
        else if (token == BOOL_LITERAL) printf(" %s", yylval.int_value ? "true" : "false");
        else if (token == IDENTIFIER) printf(" %s", yylval.identifier);
        else if (token == STRING_LITERAL) printf(" \"%s\"", yylval.string_value);
        printf("\n");
    }
    fclose(yyin);
    return 0;
}

static AstNode *parse_file(const char *path) {
    yyin = fopen(path, "r");
    if (!yyin) {
        perror(path);
        return NULL;
    }
    parsed_program = NULL;
    int rc = yyparse();
    fclose(yyin);
    if (rc != 0 || !parsed_program) return NULL;
    return parsed_program;
}

static char *executable_name_from_c(const char *path) {
    size_t len = strlen(path);
    char *out = malloc(len + 5);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    strcpy(out, path);
    char *dot = strrchr(out, '.');
    if (dot) *dot = '\0';
    strcat(out, ".exe");
    return out;
}

static int compile_c_file(const char *c_path) {
    char *exe = executable_name_from_c(c_path);
    char command[1024];
    snprintf(command, sizeof(command), "gcc -std=c99 -Wall -Wextra \"%s\" -o \"%s\"", c_path, exe);
    int rc = system(command);
    if (rc == 0) {
        printf("built %s\n", exe);
    }
    free(exe);
    return rc == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    CompileOptions opts;
    if (parse_args(argc, argv, &opts)) {
        usage(argv[0]);
        return 1;
    }

    if (opts.mode == MODE_TOKENS) {
        return print_tokens(opts.input_path);
    }

    AstNode *program = parse_file(opts.input_path);
    if (!program) return 1;

    SemanticContext *sem = semantic_context_new();
    int errors = semantic_analyze(sem, program);
    if (errors > 0) {
        fprintf(stderr, "compilation stopped after %d semantic error(s)\n", errors);
        semantic_context_free(sem);
        ast_free(program);
        return 1;
    }

    int rc = 0;
    if (opts.mode == MODE_AST) {
        ast_print(program, 0);
    } else if (opts.mode == MODE_SYMBOLS) {
        symbol_table_print(sem->symbols);
    } else if (opts.mode == MODE_TAC) {
        TacList *tac = tac_generate(program);
        tac_print(tac);
        tac_free(tac);
    } else if (opts.mode == MODE_OPT_TAC) {
        TacList *before = tac_generate(program);
        puts("Original TAC:");
        tac_print(before);
        tac_free(before);
        program = optimize_ast(program);
        TacList *after = tac_generate(program);
        puts("\nOptimized TAC:");
        tac_print(after);
        tac_free(after);
    } else if (opts.mode == MODE_ANALYZE) {
        TacList *tac = tac_generate(program);
        tac_analyze_dependencies(tac);
        tac_free(tac);
    } else {
        program = optimize_ast(program);
        rc = codegen_c_file(program, opts.output_path);
        if (rc == 0) printf("generated %s\n", opts.output_path);
        if (rc == 0 && opts.mode == MODE_COMPILE) {
            rc = compile_c_file(opts.output_path);
        }
    }

    semantic_context_free(sem);
    ast_free(program);
    return rc;
}
