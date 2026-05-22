# 03 - Analise Sintatica E AST

A analise sintatica usa Bison em `src/parser.y`. O parser valida a estrutura do programa e constroi uma AST, que representa o codigo de forma mais facil de analisar e transformar.

## Gramatica Simplificada

```ebnf
program      ::= stmt_list ;
stmt_list    ::= stmt_list statement | vazio ;

statement    ::= declaration ";"
               | assignment ";"
               | scan_stmt ";"
               | print_stmt ";"
               | block
               | if_stmt
               | while_stmt
               | for_stmt
               | ";" ;

declaration  ::= type IDENTIFIER
               | type IDENTIFIER "=" expr ;
type         ::= "int" | "bool" ;
assignment   ::= IDENTIFIER "=" expr ;

block        ::= "{" stmt_list "}" ;
if_stmt      ::= "if" "(" expr ")" statement
               | "if" "(" expr ")" statement "else" statement ;
while_stmt   ::= "while" "(" expr ")" statement ;
for_stmt     ::= "for" "(" for_init ";" expr ";" for_update ")" statement ;
for_init     ::= assignment | type IDENTIFIER "=" expr ;
for_update   ::= assignment ;

scan_stmt    ::= "scan" "(" IDENTIFIER ")" ;
print_stmt   ::= "print" "(" expr ")"
               | "print" "(" STRING_LITERAL ")" ;

expr         ::= INT_LITERAL | BOOL_LITERAL | IDENTIFIER | "(" expr ")"
               | "-" expr | "!" expr
               | expr binary_operator expr ;
```

## Precedencia

Do menor para o maior nivel: `||`, `&&`, igualdade, comparacoes, soma/subtracao, multiplicacao/divisao/modulo e operadores unarios.

## AST

A AST possui nos para programa, bloco, declaracao, atribuicao, controle de fluxo, entrada/saida, literais, identificadores e expressoes. Depois da analise semantica, alguns nos tambem recebem tipo inferido, nome seguro em C e endereco logico.
