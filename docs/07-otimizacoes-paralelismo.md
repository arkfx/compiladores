# 07 - Otimizacoes, Paralelismo E Localidade

O projeto aplica otimizacoes independentes de maquina e tambem possui um modo de analise de dependencias.

## Otimizacoes Implementadas

- Constant folding: calcula expressoes constantes, como `2 + 3 * 4`.
- Simplificacao algebrica: remove casos como `x + 0` e `x * 1`.
- Propagacao conservadora de constantes no TAC.
- Remocao simples de codigo morto para temporarios sem uso.
- Remocao de trechos inalcançaveis simples apos `goto`.
- Remocao de `if`, `while` e `for` com condicao constante falsa na AST.

## Paralelismo De Instrucao

O modo `--analyze` imprime dependencias entre instrucoes proximas do TAC. Duas instrucoes independentes sao candidatas teoricas a execucao paralela, pois nao leem/escrevem os mesmos valores.

## Localidade

A linguagem nao possui arrays ou heap, entao nao ha muito espaco para otimizar localidade de memoria na pratica. A discussao fica teorica: em linguagens maiores, o compilador poderia reordenar lacos, reduzir acessos repetidos e melhorar uso de cache.
