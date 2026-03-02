/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export const dlinit = (
  hostInstance,
  pluginInstance,
  table,
  csoundInstance,
  pluginTable = table,
  hostMemory = undefined,
) => {
  const hostExports = hostInstance && hostInstance.exports ? hostInstance.exports : {};

  if (pluginInstance.exports["csoundModuleInit"]) {
    if (typeof hostExports["csoundWasiLoadPlugin"] !== "function") {
      if (typeof pluginInstance.exports["csoundModuleCreate"] === "function") {
        pluginInstance.exports["csoundModuleCreate"](csoundInstance);
      }
      pluginInstance.exports["csoundModuleInit"](csoundInstance);
      return;
    }

    const csoundModuleCreate = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundModuleInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundModuleDestroy = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundModuleErrorCodeToString = new WebAssembly.Global(
      { value: "i32", mutable: true },
      0,
    );

    let tableEnd = table.length;

    if (typeof pluginInstance.exports["csoundModuleCreate"] === "function") {
      table.grow(1);
      csoundModuleCreate.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports["csoundModuleCreate"]);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports["csoundModuleInit"] === "function") {
      table.grow(1);
      csoundModuleInit.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports["csoundModuleInit"]);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports["csoundModuleDestroy"] === "function") {
      table.grow(1);
      csoundModuleDestroy.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports["csoundModuleDestroy"]);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports["csoundModuleErrorCodeToString"] === "function") {
      table.grow(1);
      csoundModuleErrorCodeToString.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports["csoundModuleErrorCodeToString"]);
      tableEnd += 1;
    }

    hostExports["csoundWasiLoadPlugin"](
      csoundInstance,
      csoundModuleCreate,
      csoundModuleInit,
      csoundModuleDestroy,
      csoundModuleErrorCodeToString,
    );
  } else if (
    pluginInstance.exports["csound_opcode_init"] ||
    pluginInstance.exports["csound_fgen_init"]
  ) {
    if (typeof hostExports["csoundWasiLoadOpcodeLibrary"] !== "function") {
      const appendOpcodes = hostExports["csoundAppendOpcodes"];
      const allocStringMem = hostExports["allocStringMem"];
      const freeStringMem = hostExports["freeStringMem"];
      const memory = hostMemory || hostExports["memory"];
      const missing = [];
      if (typeof appendOpcodes !== "function") {
        missing.push("csoundAppendOpcodes");
      }
      if (typeof allocStringMem !== "function") {
        missing.push("allocStringMem");
      }
      if (typeof freeStringMem !== "function") {
        missing.push("freeStringMem");
      }
      if (!memory) {
        missing.push("memory");
      }

      // LINKAGE() returns sizeof(localops) in bytes; on wasm32 OENTRY is 40 bytes.
      const wasm32OentrySize = 40;

      if (typeof pluginInstance.exports["csound_opcode_init"] === "function" && missing.length === 0) {
        const epPointerPtr = allocStringMem(4);
        try {
          const bytesRaw = pluginInstance.exports["csound_opcode_init"](csoundInstance, epPointerPtr);
          const bytes = Number(bytesRaw);
          const opcodeListPtr = new DataView(memory.buffer).getUint32(epPointerPtr, true);
          const entryCount = Math.floor(bytes / wasm32OentrySize);
          if (entryCount > 0 && opcodeListPtr !== 0) {
            const patchedOpcodeListPtr = allocStringMem(entryCount * wasm32OentrySize);
            const dv = new DataView(memory.buffer);
            const remapToHostTable = (funcIndex) => {
              if (!funcIndex) {
                return 0;
              }
              const funcRef = pluginTable.get(funcIndex);
              if (!funcRef) {
                console.error(`Missing plugin function at table index ${funcIndex}`);
                return 0;
              }
              const hostTableIndex = table.length;
              table.grow(1);
              table.set(hostTableIndex, funcRef);
              return hostTableIndex;
            };

            try {
              for (let entryIndex = 0; entryIndex < entryCount; entryIndex += 1) {
                const sourceOffset = opcodeListPtr + entryIndex * wasm32OentrySize;
                const targetOffset = patchedOpcodeListPtr + entryIndex * wasm32OentrySize;

                // OENTRY layout (wasm32): 10 x 4-byte fields.
                const opname = dv.getUint32(sourceOffset + 0, true);
                const dsblksiz = dv.getUint32(sourceOffset + 4, true);
                const flags = dv.getInt32(sourceOffset + 8, true);
                const outypes = dv.getUint32(sourceOffset + 12, true);
                const intypes = dv.getUint32(sourceOffset + 16, true);
                const init = dv.getUint32(sourceOffset + 20, true);
                const perf = dv.getUint32(sourceOffset + 24, true);
                const deinit = dv.getUint32(sourceOffset + 28, true);
                const useropinfo = dv.getUint32(sourceOffset + 32, true);
                const deprecated = dv.getInt32(sourceOffset + 36, true);

                dv.setUint32(targetOffset + 0, opname, true);
                dv.setUint32(targetOffset + 4, dsblksiz, true);
                dv.setInt32(targetOffset + 8, flags, true);
                dv.setUint32(targetOffset + 12, outypes, true);
                dv.setUint32(targetOffset + 16, intypes, true);
                dv.setUint32(targetOffset + 20, remapToHostTable(init), true);
                dv.setUint32(targetOffset + 24, remapToHostTable(perf), true);
                dv.setUint32(targetOffset + 28, remapToHostTable(deinit), true);
                dv.setUint32(targetOffset + 32, useropinfo, true);
                dv.setInt32(targetOffset + 36, deprecated, true);
              }

              appendOpcodes(csoundInstance, patchedOpcodeListPtr, entryCount);
            } finally {
              freeStringMem(patchedOpcodeListPtr);
            }
          } else {
            console.error("Invalid opcode table returned by csound_opcode_init");
          }
        } finally {
          freeStringMem(epPointerPtr);
        }
      } else if (typeof pluginInstance.exports["csound_opcode_init"] === "function") {
        console.error(`Missing required host exports for opcode plugin loading: ${missing.join(", ")}`);
      }

      if (typeof pluginInstance.exports["csound_fgen_init"] === "function") {
        console.warn("csound_fgen_init plugins are not supported by the current WASM loader path.");
      }
      return;
    }

    const csoundOpcodeInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundFgenInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);

    let tableEnd = table.length;

    if (typeof pluginInstance.exports["csound_opcode_init"] === "function") {
      csoundOpcodeInit.value = tableEnd;
      table.grow(1);
      table.set(tableEnd, pluginInstance.exports["csound_opcode_init"]);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports["csound_fgen_init"] === "function") {
      csoundFgenInit.value = tableEnd;
      table.grow(1);
      table.set(tableEnd, pluginInstance.exports["csound_fgen_init"]);
      tableEnd += 1;
    }

    hostExports["csoundWasiLoadOpcodeLibrary"](
      csoundInstance,
      csoundFgenInit,
      csoundOpcodeInit,
    );
  } else {
    console.error("Plugin doesn't export nececcary functions to quality as csound plugin.");
  }
};
