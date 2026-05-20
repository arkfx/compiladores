# MiniLang Language

MiniLang is a small imperative language for a compiler course project. A source file is compiled as the body of a generated C `main` function.

## Types

- `int`
- `bool`
- string literals only inside `print`

## Statements

```c
int x;
int y = 3;
bool ok = true;

x = y + 2;
scan(x);
print(x);
print("hello");

if (x > 0) {
    print("positive");
} else {
    print("not positive");
}

while (x > 0) {
    x = x - 1;
}

for (int i = 0; i < 10; i = i + 1) {
    print(i);
}
```

## Operators

- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logic: `&&`, `||`, `!`

## Restrictions

There are no user-defined functions, arrays, floats, string variables, `break`, `continue`, or `return` in v1.
