# Compilador miniC

miniC e uma linguagem pequena, imperativa e estruturada, criada para o trabalho de Compiladores. O projeto implementa o caminho completo de um compilador: analise lexica com Flex, analise sintatica com Bison, AST, analise semantica, tabela de simbolos, codigo intermediario em tres enderecos, otimizacoes e geracao de C99.

## Como Compilar

No ambiente Windows usado no projeto:

```sh
make
```

O `Makefile` usa `win_flex`, `win_bison` e `gcc`. Em Linux, MSYS2 ou WSL, os nomes das ferramentas podem ser sobrescritos:

```sh
make FLEX=flex BISON=bison
```

Os arquivos gerados por Flex/Bison ficam em `build/generated/`, os objetos ficam em `build/obj/`, e os headers manuais ficam em `src/include/`.

## Como Usar

```sh
./minic examples/valid/for_sum.mc
./minic examples/valid/for_sum.mc -o program.c
./minic --compile examples/valid/for_sum.mc -o program.c
```

Modos de inspecao do compilador:

```sh
./minic --tokens examples/valid/arithmetic.mc
./minic --ast examples/valid/condition.mc
./minic --symbols examples/valid/condition.mc
./minic --tac examples/valid/for_sum.mc
./minic --opt-tac examples/valid/optimization.mc
./minic --analyze examples/valid/for_sum.mc
```

## Linguagem

miniC usa a extensao `.mc` e suporta:

- tipos `int` e `bool`;
- declaracoes e atribuicoes;
- blocos com escopo;
- `if`, `else`, `while` e `for`;
- entrada com `scan`;
- saida com `print`;
- expressoes aritmeticas, relacionais e logicas.

Literais de string sao aceitos apenas como argumento direto de `print`.

## Documentacao

A pasta `docs/` esta organizada pelas etapas de um compilador:

- `01-visao-geral.md`: linguagem, paradigma e exemplos.
- `02-lexico-automatos.md`: tokens, expressoes regulares e automatos.
- `03-sintatico-ast.md`: gramatica, parser e AST.
- `04-semantica-ambiente.md`: tipos, escopos, tabela de simbolos e ambiente de execucao.
- `05-tac.md`: codigo intermediario em tres enderecos.
- `06-geracao-c.md`: geracao de C a partir do TAC.
- `07-otimizacoes-paralelismo.md`: otimizacoes, dependencias, paralelismo e localidade.
- `08-aplicacoes.md`: relacao com compiladores reais.
