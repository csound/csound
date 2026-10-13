# Platform builds

- `android/`: Android NDK build, Java bindings, and example apps.
- `androidcross/`: Android cross-build toolchain and Docker image.
- `bela/`, `daisy/`, `esp32/`, `zynq/`: board builds.
- `ios/`: native iOS build and example apps.
- `ioscross/`: iOS cross-build toolchains and Docker image.
- `wasm-wasi/`: WASI binaries, browser and Node.js packages, and Nix builds.
- `wasm-emscripten/`: Emscripten toolchain and Docker image.
- `dockerfiles/`: older Docker recipes, including Csound 6 builds.
- `vcpkg/`: dependency ports and triplets. The manifests `vcpkg.json` and
  `vcpkg-x86.json`, along with the `vcpkg/` submodule, live at the repository root.

The other folders hold desktop build and packaging files.
Language bindings live in [`languages/`](../languages/README.md).
