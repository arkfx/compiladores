# Lexical Automata Notes

The lexer is implemented with Flex regular expressions.

## Identifier Automaton

Start in `q0`. If the first character is a letter or `_`, move to accepting state `q1`. Stay in `q1` while reading letters, digits, or `_`. Keywords are checked before identifiers.

Regular expression:

```text
[A-Za-z_][A-Za-z0-9_]*
```

## Integer Automaton

Start in `q0`. A digit moves to accepting state `q1`. Stay in `q1` while reading digits.

Regular expression:

```text
[0-9]+
```

## String Literal Automaton

Start after `"`. Accept normal non-newline characters or escaped characters until the closing `"`.

Regular expression:

```text
\"([^\\\"\n]|\\.)*\"
```

## Operators And Delimiters

Two-character operators are matched before one-character operators: `==`, `!=`, `<=`, `>=`, `&&`, `||`.
