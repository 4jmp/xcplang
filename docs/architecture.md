# Architecture

xcplang has a small front end and a native execution core.

1. The lexer reads source characters and creates tokens.
2. The recursive-descent parser creates expression and statement nodes.
3. The semantic layer owns names and parent-linked scopes.
4. The runtime evaluates the parsed syntax tree directly.
5. Runtime objects and the GC manage strings and arrays.
6. The module loader reads local `.xcp` files.

The repository also contains bytecode and VM modules, but the CLI does not use
them yet. There is no JIT compiler.

The driver connects the CLI to the module loader. The Rust crate exposes a small C ABI for safe native services and future VM work.
