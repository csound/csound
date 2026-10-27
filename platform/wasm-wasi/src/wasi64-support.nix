{ system ? builtins.currentSystem, pkgs ? import <nixpkgs> {inherit system;} }:
pkgs.runCommand "csound-wasip2-memory64-support" {
  nativeBuildInputs = [pkgs.wasm-tools pkgs.wasmtime];
} ''
  mkdir -p "$out"
  wasm-tools validate --features=cm64 ${../tests/preview2/string64.wat}
  wasmtime run -Ccache=n --invoke 'get-string()' ${../tests/preview2/string32.wat} > control.log
  test "$(cat control.log)" = '"ok"'
  if wasmtime run -Ccache=n -Wcomponent-model-memory64=y --invoke 'get-string()' \
      ${../tests/preview2/string64.wat} > memory64.log 2>&1 && \
      test "$(cat memory64.log)" = '"ok"'; then
    echo 1 > "$out/supported"
  else
    echo 0 > "$out/supported"
  fi
  cp control.log memory64.log "$out/"
''
