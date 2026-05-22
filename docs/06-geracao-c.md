# 06 - Geracao De Codigo C

A etapa final traduz o TAC otimizado para C99. Isso mostra que o codigo intermediario nao e apenas demonstrativo: ele participa do caminho real ate o codigo final.

## Estrutura Gerada

O backend C:

- inclui `stdio.h` e `stdbool.h`;
- declara variaveis e temporarios no inicio de `main`;
- emite atribuicoes e operacoes a partir das instrucoes TAC;
- traduz labels e saltos para labels e `goto` em C;
- traduz `scan` para `scanf`;
- traduz `print` para `printf`.

## Por Que Usar Labels E Goto

O uso de labels e `goto` deixa clara a traducao de estruturas de controle para fluxo de baixo nivel, que e um conceito importante em compiladores.
