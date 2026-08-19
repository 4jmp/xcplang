# Troubleshooting

If `make` cannot find a compiler, install a C++20 compiler and try again. If the Rust check cannot find Cargo, install Rust from the official Rust project.

If a program is rejected, check that its name ends in `.xcp`. If an imported module is not found, check its relative path and file extension.

For a clean rebuild, run `make clean && make`.
