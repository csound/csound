/*
  wasm_plugins.h:

  Internal interface for loading wasm32 opcode libraries with Wasmtime.
*/

#ifndef CSOUND_WASM_PLUGINS_H
#define CSOUND_WASM_PLUGINS_H

#include "csoundCore.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t csoundLoadWasmOpcodeLibrary(CSOUND *csound, const char *path);
int32_t csoundInitWasmOpcodeLibraries(CSOUND *csound);
int32_t csoundDestroyWasmOpcodeLibraries(CSOUND *csound);

#ifdef __cplusplus
}
#endif

#endif
