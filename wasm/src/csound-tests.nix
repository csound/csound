{
  system ?
    if builtins.match ".*-darwin" builtins.currentSystem != null
    then "x86_64-linux"
    else builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  csoundWasm ? ../lib/csound.wasm,
  commandlineTests ? ../../tests/commandline,
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

  cp ${csoundWasm} csound.wasm
  wasmtime compile -Wexceptions=y -Ccache=n -o csound.cwasm csound.wasm
  csound_module="$PWD/csound.cwasm"

  cp -R ${commandlineTests} commandline-tests
  chmod -R u+w commandline-tests
  cd commandline-tests

  python3 test.py \
    --runtime-executable=wasmtime \
    --runtime-arg=run \
    --runtime-arg=-Wexceptions=y \
    --runtime-arg=-Ccache=n \
    --runtime-arg=--allow-precompiled \
    --runtime-arg=--dir=. \
    --expected-failure=test_osc_server.csd \
    --csound-executable="$csound_module" \
    --source-dir=. \
    --workers=${toString workers} \
    --timeout=${toString testTimeout}

  mkdir -p "$out"
  cp results.txt "$out/"
''
