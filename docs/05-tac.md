# 05 - Codigo Intermediario TAC

Depois da AST e da analise semantica, o compilador gera TAC, isto e, codigo de tres enderecos.

## Ideia

Expressoes complexas sao quebradas em instrucoes menores com temporarios:

```text
mc_t0 = a + b
mc_t1 = mc_t0 * 2
c = mc_t1
```

## Instrucoes

O TAC do projeto possui instrucoes estruturadas para:

- declaracao;
- atribuicao;
- operacao binaria;
- operacao unaria;
- label;
- `goto`;
- salto condicional `ifFalse`;
- `scan`;
- `print`.

O modo `--tac` mostra a representacao intermediaria legivel. O modo `--opt-tac` mostra a diferenca antes e depois das otimizacoes.
