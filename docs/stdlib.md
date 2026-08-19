# Standard library

The native standard library is in `src/stdlib/`.

- `print(value...)` writes values followed by a newline.
- `input()` reads one line from standard input.
- `len(value)` returns text or array length.
- `absolute(number)` returns the absolute number.

The I/O functions use the host terminal. They do not store hidden global state.
