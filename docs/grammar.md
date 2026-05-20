# MiniLang Grammar

```ebnf
program      ::= stmt_list ;
stmt_list    ::= stmt_list statement | empty ;

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

Operator precedence is implemented in Bison from lowest to highest: `||`, `&&`, equality, comparisons, addition/subtraction, multiplication/division/modulo, unary operators.
