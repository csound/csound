{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  csoundWasm ? ../test-artifacts/csound-realtime.wasm,
  commandlineTests ? ../../../tests/commandline,
  workers ? 4,
  testTimeout ? 60,
}:
pkgs.runCommand "csound-wasm-commandline-tests" {
  nativeBuildInputs = [
    pkgs.python3
    pkgs.wasmtime
  ];
} ''
  export HOME="$TMPDIR"

  cp ${csoundWasm} csound-realtime.wasm
  wasmtime compile -Wthreads=y -Wshared-memory=y -Wexceptions=y \
    -Ccache=n -o csound.cwasm csound-realtime.wasm
  csound_module="$PWD/csound.cwasm"

  cp -R ${commandlineTests} commandline-tests
  chmod -R u+w commandline-tests
  cd commandline-tests

  # CSDs select --realtime themselves; enable WASI threads for those that do.
  python3 test.py \
    --runtime-executable=wasmtime \
    --runtime-arg=run \
    --runtime-arg=-Sthreads=y \
    --runtime-arg=-Wthreads=y \
    --runtime-arg=-Wshared-memory=y \
    --runtime-arg=-Wexceptions=y \
    --runtime-arg=-Ccache=n \
    --runtime-arg=--allow-precompiled \
    --runtime-arg=--dir=. \
    --csound-executable="$csound_module" \
    --source-dir=. \
    --workers=${toString workers} \
    --timeout=${toString testTimeout}

  mkdir -p "$out"
  cp results.txt "$out/"
''
