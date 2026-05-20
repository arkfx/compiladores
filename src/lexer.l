%option noyywrap yylineno nounput noinput
%x COMMENT

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"

int yycolumn = 1;

static char *copy_text(const char *text, int len) {
    char *copy = malloc((size_t)len + 1);
    if (!copy) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memcpy(copy, text, (size_t)len);
    copy[len] = '\0';
    return copy;
}

static char *unquote_string(const char *text) {
    int len = (int)strlen(text);
    char *out = malloc((size_t)len);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    int j = 0;
    for (int i = 1; i < len - 1; i++) {
        if (text[i] == '\\' && i + 1 < len - 1) {
            i++;
            if (text[i] == 'n') out[j++] = '\n';
            else if (text[i] == 't') out[j++] = '\t';
            else out[j++] = text[i];
        } else {
            out[j++] = text[i];
        }
    }
    out[j] = '\0';
    return out;
}

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + (int)yyleng - 1; \
    yycolumn += (int)yyleng;
%}

%%

"//".*                  ;
"/*"                    { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { yycolumn = 1; }
<COMMENT>.              ;
[ \t\r]+                ;
\n                      { yycolumn = 1; }

"int"                   { return KW_INT; }
"bool"                  { return KW_BOOL; }
"if"                    { return KW_IF; }
"else"                  { return KW_ELSE; }
"while"                 { return KW_WHILE; }
"for"                   { return KW_FOR; }
"scan"                  { return KW_SCAN; }
"print"                 { return KW_PRINT; }
"true"                  { yylval.int_value = 1; return BOOL_LITERAL; }
"false"                 { yylval.int_value = 0; return BOOL_LITERAL; }

"=="                    { return EQ; }
"!="                    { return NEQ; }
"<="                    { return LTE; }
">="                    { return GTE; }
"&&"                    { return AND; }
"||"                    { return OR; }

[0-9]+                  { yylval.int_value = atoi(yytext); return INT_LITERAL; }
\"([^\\\"\n]|\\.)*\"    { yylval.string_value = unquote_string(yytext); return STRING_LITERAL; }
[A-Za-z_][A-Za-z0-9_]*  { yylval.identifier = copy_text(yytext, (int)yyleng); return IDENTIFIER; }

[{}();=+\-*/%<>!]       { return yytext[0]; }

.                       {
                            fprintf(stderr, "lexical error at line %d, column %d: unexpected character '%s'\n",
                                    yylineno, yycolumn, yytext);
                        }

%%
