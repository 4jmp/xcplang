# Getting started

## Requirements

You need a C++20 compiler and GNU Make. Meson and Ninja are optional. Rust is needed only for the FFI bridge.

## Build

Run `make` in the project root. It creates `xcp`. Run `make test` for the smoke test.

## Run code

Use `xcp program.xcp`, `xcp run program.xcp`, or `xcp repl`. The REPL reads one line at a time. Use `xcp help` for the command list.
