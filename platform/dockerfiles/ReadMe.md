# Docker recipes

These older recipes remain for reference. Some clone the `csound6` branch,
so their source paths refer to that branch.

To build and run a recipe from the repository root:

```sh
docker build -t csound-local platform/dockerfiles/linux
docker run --rm -it csound-local
```

Current CI builds use the workflows in [`.github/workflows/`](../../.github/workflows).
The Android and Emscripten cross-build images live in
[`androidcross/`](../androidcross/README.md) and
[`wasm-emscripten/`](../wasm-emscripten/README.md).
