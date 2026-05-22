# 04 - Analise Semantica E Ambiente De Execucao

A analise semantica verifica se o programa faz sentido depois de estar sintaticamente correto.

## Verificacoes

- Variavel deve ser declarada antes do uso.
- Nao pode haver duas declaracoes com o mesmo nome no mesmo escopo.
- Atribuicoes devem respeitar tipos.
- Condicoes de `if`, `while` e `for` devem ser `bool`.
- Operadores aritmeticos recebem `int`.
- Operadores logicos recebem `bool`.
- `scan` aceita apenas variaveis `int` ou `bool`.

## Tabela De Simbolos

Cada simbolo armazena:

- nome original;
- tipo;
- escopo;
- linha de declaracao;
- endereco logico;
- nome seguro gerado para C.

O endereco logico simula a posicao da variavel no ambiente de execucao. O nome seguro em C evita conflito com funcoes e palavras reservadas, por exemplo uma variavel chamada `printf` vira algo como `mc_0_0_printf`.

## Escopos

Blocos `{ ... }` e inicializacoes de `for` criam escopos. Variaveis internas podem sombrear variaveis externas, e o compilador gera nomes C diferentes para cada simbolo.
