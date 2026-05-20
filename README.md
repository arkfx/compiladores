# MiniLang Compiler

MiniLang is a small compiler project for a compiler course. It uses Flex and Bison, builds an AST, performs semantic analysis, emits three-address code, applies simple optimizations, and generates C99 code.

## Build

On this Windows setup:

```sh
make
```

The Makefile uses `win_flex`, `win_bison`, and `gcc`. On Linux/MSYS2/WSL, override the tool names if needed:

```sh
make FLEX=flex BISON=bison
```

Generated Flex/Bison files and object files are written under `build/`, keeping `src/` reserved for handwritten source files and grammar/lexer specs.

## Run

```sh
./minilang examples/valid/for_sum.ml
./minilang examples/valid/for_sum.ml -o program.c
./minilang --compile examples/valid/for_sum.ml -o program.c
```

Debug/demo modes:

```sh
./minilang --tokens examples/valid/arithmetic.ml
./minilang --ast examples/valid/condition.ml
./minilang --symbols examples/valid/condition.ml
./minilang --tac examples/valid/for_sum.ml
./minilang --opt-tac examples/valid/optimization.ml
./minilang --analyze examples/valid/for_sum.ml
```

## Language

MiniLang supports `int`, `bool`, declarations, assignments, blocks, `if`, `else`, `while`, `for`, `scan`, and `print`. String literals are supported only as direct `print` arguments.
