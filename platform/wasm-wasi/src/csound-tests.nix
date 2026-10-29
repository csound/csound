{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> {inherit system;},
  threaded ? false,
  csoundWasm ? if threaded then ../test-artifacts/csound-realtime.wasm else ../lib/csound-wasm64-cli.wasm,
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

  cp ${csoundWasm} csound-command.wasm
  wasmtime compile ${pkgs.lib.optionalString threaded "-Wthreads=y -Wshared-memory=y"} -Wexceptions=y \
    -Ccache=n -o csound.cwasm csound-command.wasm
  csound_module="$PWD/csound.cwasm"

  cp -R ${commandlineTests} commandline-tests
  chmod -R u+w commandline-tests
  cd commandline-tests

  # Enable native threads only when the toolchain can build that command.
  python3 test.py \
    --runtime-executable=wasmtime \
    --runtime-arg=run \
    ${pkgs.lib.optionalString threaded "--runtime-arg=-Sthreads=y --runtime-arg=-Wthreads=y --runtime-arg=-Wshared-memory=y"} \
    --runtime-arg=-Wexceptions=y \
    --runtime-arg=-Ccache=n \
    --runtime-arg=--allow-precompiled \
    --runtime-arg=--dir=. \
    --csound-executable="$csound_module" \
    --profile=${if threaded then "wasm" else "wasip2"} \
    --source-dir=. \
    --workers=${toString workers} \
    --timeout=${toString testTimeout}

  mkdir -p "$out"
  cp results.txt "$out/"
''
