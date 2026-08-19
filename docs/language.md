# Language guide

## Values and variables

Use `let` to create a variable. Numbers, strings, booleans, and `null` are built-in values.

```xcp
let count = 10
let title = "hello"
let ready = true
print(title, count, ready)
```

## Expressions

The operators `+`, `-`, `*`, and `/` work with numbers. The plus operator also joins text.

```xcp
let total = 2 + 3 * 4
print("total", total)
```

## Conditions

Use `if` and `else` for a branch. A block is surrounded by braces.

```xcp
if (true) { print("ready") } else { print("wait") }
```

## Built-ins

`print` writes values, `len` returns text length, and `input` reads one line from the terminal.
