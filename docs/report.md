# MiniLang Compiler Report Notes

## Pipeline

MiniLang uses Flex for lexical analysis and Bison for LR parsing. The parser builds an AST. A semantic pass annotates expressions with types and validates scopes using a symbol table. The compiler can print TAC, optimize the AST, and generate C99 code.

## Symbol Table

Each symbol stores name, type, scope depth, declaration line, and generated C name. Blocks and `for` statements create nested scopes. Inner blocks may shadow outer variables.

## Intermediate Code

The TAC representation uses temporaries `t0`, `t1`, ... and labels `L0`, `L1`, ... . Conditional control flow is represented with `ifFalse` and `goto`.

## Optimizations

Implemented optimizations:

- Constant folding
- Algebraic simplification
- Removal of branches and loops with constant false conditions

## Parallelism And Locality

The `--analyze` mode prints nearby TAC instruction dependency information. Independent instructions are candidates for instruction-level parallelism. Since the language has no arrays or heap memory, locality optimization is discussed theoretically through loop structure and redundant expression reduction.

## Applications

The project connects to real compiler uses such as JIT compilers, GPU compilers, embedded-code optimization, and source-to-source compilation.
