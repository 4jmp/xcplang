# Command line reference

`xcp <file.xcp>` runs a file. `xcp run <file.xcp>` uses the same loader. `xcp repl` starts the interactive prompt. `xcp help` prints usage. `xcp --version` and `xcp -v` print the release version. Use `--allow-destructive` or `-d` only when a file is allowed to run destructive shell commands.

A missing file, an invalid extension, or a source error returns a non-zero status.
