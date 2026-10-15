#!/usr/bin/env bash
set -euo pipefail

# Supply the compiler and linker from one LLVM version. No WASI sysroot or
# pthread runtime is used: this checks only Wasm atomics and shared memory.
probe_dir=$(cd "$(dirname "$0")" && pwd)
probe_build=$(mktemp -d)
trap 'rm -rf "$probe_build"' EXIT

"${CC:-cc}" -std=c11 -O2 -Wall -Wextra "$probe_dir/spinlock.c" -o "$probe_build/spinlock"
"$probe_build/spinlock"
"${WASM_CC:-clang}" --target=wasm32 -O2 -std=c11 -ffreestanding -nostdlib \
  -matomics -mbulk-memory -c "$probe_dir/queue.c" -o "$probe_build/queue.o"
"${WASM_LD:-wasm-ld}" --no-entry --shared-memory --import-memory \
  --initial-memory=262144 --max-memory=262144 -z stack-size=65536 \
  --export=queue_reset --export=queue_push --export=queue_pop \
  "$probe_build/queue.o" -o "$probe_build/queue.wasm"
"${NODE:-node}" "$probe_dir/queue.mjs" "$probe_build/queue.wasm"
