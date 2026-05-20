%{
#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *message);

AstNode *parsed_program = NULL;
%}

%code requires {
    #include "ast.h"
}

%locations

%union {
    int int_value;
    char *identifier;
    char *string_value;
    TypeKind type;
    AstNode *node;
    AstList *list;
}

%token <int_value> INT_LITERAL BOOL_LITERAL
%token <identifier> IDENTIFIER
%token <string_value> STRING_LITERAL
%token KW_INT KW_BOOL KW_IF KW_ELSE KW_WHILE KW_FOR KW_SCAN KW_PRINT
%token EQ NEQ LTE GTE AND OR

%type <type> type
%type <node> statement declaration assignment block if_stmt while_stmt for_stmt scan_stmt print_stmt expr for_init for_update
%type <list> stmt_list

%left OR
%left AND
%left EQ NEQ
%left '<' '>' LTE GTE
%left '+' '-'
%left '*' '/' '%'
%right '!'
%right UMINUS
%nonassoc LOWER_THAN_ELSE
%nonassoc KW_ELSE

%%

program
    : stmt_list
      { parsed_program = ast_new_program($1, @1.first_line); }
    ;

stmt_list
    : /* empty */
      { $$ = ast_list_new(); }
    | stmt_list statement
      { ast_list_append($1, $2); $$ = $1; }
    ;

statement
    : declaration ';' { $$ = $1; }
    | assignment ';' { $$ = $1; }
    | scan_stmt ';' { $$ = $1; }
    | print_stmt ';' { $$ = $1; }
    | block { $$ = $1; }
    | if_stmt { $$ = $1; }
    | while_stmt { $$ = $1; }
    | for_stmt { $$ = $1; }
    | ';' { $$ = ast_new_empty(@1.first_line); }
    ;

declaration
    : type IDENTIFIER
      { $$ = ast_new_decl($1, $2, NULL, @2.first_line); }
    | type IDENTIFIER '=' expr
      { $$ = ast_new_decl($1, $2, $4, @2.first_line); }
    ;

assignment
    : IDENTIFIER '=' expr
      { $$ = ast_new_assign($1, $3, @1.first_line); }
    ;

block
    : '{' stmt_list '}'
      { $$ = ast_new_block($2, @1.first_line); }
    ;

if_stmt
    : KW_IF '(' expr ')' statement %prec LOWER_THAN_ELSE
      { $$ = ast_new_if($3, $5, NULL, @1.first_line); }
    | KW_IF '(' expr ')' statement KW_ELSE statement
      { $$ = ast_new_if($3, $5, $7, @1.first_line); }
    ;

while_stmt
    : KW_WHILE '(' expr ')' statement
      { $$ = ast_new_while($3, $5, @1.first_line); }
    ;

for_stmt
    : KW_FOR '(' for_init ';' expr ';' for_update ')' statement
      { $$ = ast_new_for($3, $5, $7, $9, @1.first_line); }
    ;

for_init
    : assignment { $$ = $1; }
    | type IDENTIFIER '=' expr
      { $$ = ast_new_decl($1, $2, $4, @2.first_line); }
    ;

for_update
    : assignment { $$ = $1; }
    ;

scan_stmt
    : KW_SCAN '(' IDENTIFIER ')'
      { $$ = ast_new_scan($3, @1.first_line); }
    ;

print_stmt
    : KW_PRINT '(' expr ')'
      { $$ = ast_new_print($3, @1.first_line); }
    | KW_PRINT '(' STRING_LITERAL ')'
      { $$ = ast_new_print(ast_new_string($3, @3.first_line), @1.first_line); }
    ;

type
    : KW_INT { $$ = TYPE_INT; }
    | KW_BOOL { $$ = TYPE_BOOL; }
    ;

expr
    : INT_LITERAL { $$ = ast_new_int($1, @1.first_line); }
    | BOOL_LITERAL { $$ = ast_new_bool($1, @1.first_line); }
    | IDENTIFIER { $$ = ast_new_identifier($1, @1.first_line); }
    | '(' expr ')' { $$ = $2; }
    | '-' expr %prec UMINUS { $$ = ast_new_unary(UN_NEG, $2, @1.first_line); }
    | '!' expr { $$ = ast_new_unary(UN_NOT, $2, @1.first_line); }
    | expr '+' expr { $$ = ast_new_binary(BIN_ADD, $1, $3, @2.first_line); }
    | expr '-' expr { $$ = ast_new_binary(BIN_SUB, $1, $3, @2.first_line); }
    | expr '*' expr { $$ = ast_new_binary(BIN_MUL, $1, $3, @2.first_line); }
    | expr '/' expr { $$ = ast_new_binary(BIN_DIV, $1, $3, @2.first_line); }
    | expr '%' expr { $$ = ast_new_binary(BIN_MOD, $1, $3, @2.first_line); }
    | expr EQ expr { $$ = ast_new_binary(BIN_EQ, $1, $3, @2.first_line); }
    | expr NEQ expr { $$ = ast_new_binary(BIN_NEQ, $1, $3, @2.first_line); }
    | expr '<' expr { $$ = ast_new_binary(BIN_LT, $1, $3, @2.first_line); }
    | expr LTE expr { $$ = ast_new_binary(BIN_LTE, $1, $3, @2.first_line); }
    | expr '>' expr { $$ = ast_new_binary(BIN_GT, $1, $3, @2.first_line); }
    | expr GTE expr { $$ = ast_new_binary(BIN_GTE, $1, $3, @2.first_line); }
    | expr AND expr { $$ = ast_new_binary(BIN_AND, $1, $3, @2.first_line); }
    | expr OR expr { $$ = ast_new_binary(BIN_OR, $1, $3, @2.first_line); }
    ;

%%

void yyerror(const char *message) {
    fprintf(stderr, "syntax error at line %d: %s\n", yylloc.first_line, message);
}
