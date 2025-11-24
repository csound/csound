# Csound error-case tests

This folder contains a small set of Csound `.csd` files designed to trigger different kinds of errors (parser/compile-time and runtime file I/O). Run them with your built `csound` executable and collect stderr/stdout for inspection.

Files

- `unknown-opcode.csd` — references a non-existent opcode (compile/semantic error).
- `syntax-error.csd` — contains invalid tokens to provoke lexer/parser errors.
- `diskin-missing-file.csd` — calls `diskin2` on a clearly non-existent file to trigger file-open/runtime errors.
- `function-not-found.csd` — calls a non-existent function in an expression to trigger "function not found".
- `gen-insufficient-args.csd` — an intentionally incomplete `f` (GEN) line to provoke fgens errors.
- `malformed-array.csd` — malformed array syntax to try to trigger array/parse errors.

