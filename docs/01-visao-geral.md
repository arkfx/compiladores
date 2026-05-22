# 01 - Visao Geral Da Linguagem

miniC e uma linguagem pequena, imperativa e estruturada. Um programa `.mc` e tratado como o corpo de uma funcao `main` gerada em C.

## Recursos

- Variaveis dos tipos `int` e `bool`.
- Expressoes aritmeticas: `+`, `-`, `*`, `/`, `%`.
- Expressoes relacionais: `==`, `!=`, `<`, `<=`, `>`, `>=`.
- Expressoes logicas: `&&`, `||`, `!`.
- Controle de fluxo com `if`, `else`, `while` e `for`.
- Entrada e saida com `scan` e `print`.

## Exemplo

```c
int sum = 0;

for (int i = 1; i <= 5; i = i + 1) {
    sum = sum + i;
}

print(sum);
```

## Restricoes

miniC nao possui funcoes definidas pelo usuario, arrays, ponteiros, `break`, `continue`, `return`, strings como variaveis, nem numeros de ponto flutuante. Essas restricoes deixam o projeto pequeno o bastante para mostrar todas as etapas do compilador com clareza.
