# xcplang

xcplang is a small custom programming language. It uses a C++20 runtime and a Rust FFI bridge. The project is made for learning systems programming in a clear way.

## Install

### Build from source

Linux and BSD systems are supported.

```sh
git clone https://github.com/4jmp/xcplang.git
cd xcplang
make
./xcp --version
```

Meson users can run `meson setup build && meson compile -C build`. The Rust bridge can be checked with `cargo check`.

### Run the installer
(COMING SOON)
```sh
curl -fsSL https://erotyka.lol/download | sh
```
or run it local:
```sh
bash install.sh
```

The installer supports Linux, FreeBSD, OpenBSD, and NetBSD. It asks before installing and before changing your shell PATH. It always prints the full binary path.

## First program

Save this as `hello.xcp`:

```xcp
let name = "xcplang"
print("hello", name)
```

Run it with `xcp hello.xcp`, `xcp run hello.xcp`, or `xcp repl`.

Only files ending in `.xcp` are accepted.

## Useful commands

```xcp
print(run_bash("printf hello"))
print(fetch("https://example.com"))
print(rest_get("https://api.example.com/items"))
print(rest_post("https://api.example.com/items", "{\"name\":\"xcp\"}"))
print(ws_request("wss://echo.example.com", "hello"))
print(genai_google("say hello", key))
```

Read [docs/network.md](docs/network.md) before using network and shell functions. Shell commands run with the permissions of the current user.

## Repository layout

- `src/` is the named C++ language and runtime tree.
- `rust/` contains the native FFI bridge.
- `tests/` contains runnable language examples.
- `docs/` contains the full user and developer guide.
- `site/` contains the portal and documentation server.
- `commit-1/` through `commit-15/` are learning snapshots.

## Development

Run `make test` after code changes. Keep C++ files at two-space indentation and use snake_case names. New language behavior needs a `.xcp` test. See [docs/contributing.md](docs/contributing.md).

## License

xcplang is released under the Apache License 2.0. See [LICENSE](LICENSE).
