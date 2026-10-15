{
  system ? builtins.currentSystem,
  pkgs ? import <nixpkgs> { inherit system; },
  gitHash ? "",
  buildDate ? "",
  testHooks ? false,
}:
let
  toolchain = import ./wasi-realtime-toolchain.nix { inherit pkgs; };
  base = import ./csound.nix {
    inherit system pkgs gitHash buildDate;
    stdenvWasm = toolchain.stdenv;
    moduleKind = "browser";
  };
in base.overrideAttrs (old: {
  pname = "csound-browser-realtime";
  cmakeFlags = old.cmakeFlags ++ [
    "-DBUILD_WASI_REALTIME=ON"
    "-DBUILD_MULTI_CORE=OFF"
    "-DBUILD_WASI_REALTIME_TESTS=${if testHooks then "ON" else "OFF"}"
  ];
  nativeBuildInputs = old.nativeBuildInputs ++ [ pkgs.nodejs ];
  # Real C recovery replaces the old browser shim's trap-based jump stubs.
  NIX_CFLAGS_COMPILE = old.NIX_CFLAGS_COMPILE ++ [
    "-mllvm -wasm-use-legacy-eh=false"
  ];
  NIX_LDFLAGS = builtins.filter
    (flag: !(pkgs.lib.hasPrefix "-z,stack-size=" flag)) old.NIX_LDFLAGS ++ [
    "-z,stack-size=1048576"
    "--initial-memory=16777216"
    "--export=wasi_thread_start"
    # Side modules use this reactor's jump runtime and exception tag.
    "--export=__c_longjmp"
    "--export=__wasm_setjmp"
    "--export=__wasm_setjmp_test"
    "--export=__wasm_longjmp"
    "--export=pthread_self"
    "--export-if-defined=csoundLoadExternals"
    "--export-if-defined=csoundWasiLoadPlugin"
    "--export-if-defined=csoundWasiLoadOpcodeLibrary"
    "-lsetjmp"
  ];
  buildPhase = ''
    runHook preBuild
    ninja -j"$NIX_BUILD_CORES" csound-bin
    node --experimental-wasm-exnref ../platform/wasm-wasi/tests/realtime/check-reactor.mjs ./csound
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/lib" "$out/include/csound"
    cp csound "$out/lib/csound-browser-realtime.wasm"
    zopfli --zlib -c csound > "$out/lib/csound-browser-realtime.wasm.z"
    cp libcsound64.a "$out/lib/"
    cp ../include/*.h include/*.h "$out/include/csound/"
    mkdir -p "$out/share/csound-plugin-sdk/include" "$out/share/csound-plugin-sdk/lib"
    cp -R "$out/include/csound" "$out/share/csound-plugin-sdk/include/"
    cp "$out/lib/libcsound64.a" "$out/share/csound-plugin-sdk/lib/"
    cat > "$out/share/csound-plugin-sdk/README.md" <<'EOF'
    Csound shared-memory WASM plugin SDK

    Build side modules with the matching threaded WASI toolchain, -fPIC,
    -pthread, and standard WebAssembly exception handling. Import the engine's
    memory, function table, stack pointer, allocator, and C runtime. Export
    __wasm_init_tls, __tls_size, and __tls_align for each worker's TLS.
    Load plugins before playback; run constructors and data relocations once.
    EOF
    tar -C "$out/share" -czf "$out/lib/csound-plugin-sdk-shared.tar.gz" csound-plugin-sdk
    runHook postInstall
  '';
  postInstall = "";
  passthru = (old.passthru or {}) // { inherit toolchain; };
})
