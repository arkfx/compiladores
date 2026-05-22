# 02 - Analise Lexica E Automatos

A analise lexica e feita com Flex em `src/lexer.l`. O lexer transforma o texto fonte em tokens para o parser.

## Tokens

- Palavras-chave: `int`, `bool`, `if`, `else`, `while`, `for`, `scan`, `print`, `true`, `false`.
- Identificadores: nomes de variaveis.
- Numeros inteiros.
- Strings literais para `print`.
- Operadores e delimitadores: `{`, `}`, `(`, `)`, `;`, `=`, `+`, `-`, `*`, `/`, `%`, comparadores e operadores logicos.

## Automato De Identificadores

Expressao regular:

```text
[A-Za-z_][A-Za-z0-9_]*
```

O automato sai de `q0` ao ler uma letra ou `_` e vai para `q1`, que e estado final. Em `q1`, ele continua aceitando letras, digitos e `_`.

## Automato De Inteiros

Expressao regular:

```text
[0-9]+
```

Ao ler o primeiro digito, o automato entra em estado final e permanece nele enquanto continuar lendo digitos.

## Automato De Strings

Expressao regular:

```text
\"([^\\\"\n]|\\.)*\"
```

O automato inicia depois de `"`, aceita caracteres normais ou escapados, e termina quando encontra o `"` final.

## Tratamento De Erros

Caracteres desconhecidos incrementam um contador de erros lexicos. Se houver erro, o compilador encerra com codigo diferente de zero e nao considera a compilacao valida.
