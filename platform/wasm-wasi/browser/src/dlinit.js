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

import { addressNumber } from "./utils/wasm64.js";

const tableFunctionIndexes = new WeakMap();

const internTableFunction = (table, func) => {
  let indexes = tableFunctionIndexes.get(table);
  if (!indexes) {
    indexes = new WeakMap();
    tableFunctionIndexes.set(table, indexes);
  }

  let index = indexes.get(func);
  if (index === undefined || table.get(index) !== func) {
    index = table.length;
    table.grow(1);
    table.set(index, func);
    indexes.set(func, index);
  }
  return index;
};

export const dlinit = (
  hostInstance,
  pluginInstance,
  table,
  csoundInstance,
  pluginTable = table,
  hostMemory = undefined,
  internFunction = internTableFunction,
) => {
  const hostExports = hostInstance && hostInstance.exports ? hostInstance.exports : {};
  const info =
    typeof pluginInstance.exports["csoundModuleInfo"] === "function"
      ? pluginInstance.exports["csoundModuleInfo"]()
      : 0;
  // Match the native loader: missing metadata denotes the legacy cs_double ABI.
  const getSampleSize =
    hostExports["csoundGetSizeOfCsFloat"] || hostExports["csoundGetSizeOfMYFLT"];
  const sampleSize = info & 0x7f;
  const doubleSize =
    typeof hostExports["csoundGetSizeOfCsDouble"] === "function"
      ? hostExports["csoundGetSizeOfCsDouble"]()
      : 8;
  if (Boolean(info & 0x80) !== (doubleSize === 4)) {
    throw new Error("Incompatible plugin cs_double precision; rebuild with matching USE_FLOAT");
  }
  if (sampleSize && typeof getSampleSize === "function" && sampleSize !== getSampleSize()) {
    throw new Error("Incompatible plugin cs_float precision; rebuild with matching USE_DOUBLE");
  }

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

    if (typeof pluginInstance.exports["csoundModuleCreate"] === "function") {
      csoundModuleCreate.value = internFunction(
        table,
        pluginInstance.exports["csoundModuleCreate"],
      );
    }

    if (typeof pluginInstance.exports["csoundModuleInit"] === "function") {
      csoundModuleInit.value = internFunction(table, pluginInstance.exports["csoundModuleInit"]);
    }

    if (typeof pluginInstance.exports["csoundModuleDestroy"] === "function") {
      csoundModuleDestroy.value = internFunction(
        table,
        pluginInstance.exports["csoundModuleDestroy"],
      );
    }

    if (typeof pluginInstance.exports["csoundModuleErrorCodeToString"] === "function") {
      csoundModuleErrorCodeToString.value = internFunction(
        table,
        pluginInstance.exports["csoundModuleErrorCodeToString"],
      );
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
      const allocByteMem = hostExports["allocByteMem"];
      const freeByteMem = hostExports["freeByteMem"];
      const memory = hostMemory || hostExports["memory"];
      const missing = [];
      if (typeof appendOpcodes !== "function") {
        missing.push("csoundAppendOpcodes");
      }
      if (typeof allocByteMem !== "function") {
        missing.push("allocByteMem");
      }
      if (typeof freeByteMem !== "function") {
        missing.push("freeByteMem");
      }
      if (!memory) {
        missing.push("memory");
      }

      // Use the engine's native layout; old wasm32 engines keep the frozen ABI.
      const pointerBytes = hostExports["__pointerSize"] || 4;
      const entrySize = hostExports["csoundWasiOentrySize"]?.() || 40;
      const functionOffsets = [0, 1, 2].map(
        (index) => hostExports["csoundWasiOentryFunctionOffset"]?.(index) ?? 20 + index * 4,
      );

      if (
        typeof pluginInstance.exports["csound_opcode_init"] === "function" &&
        missing.length === 0
      ) {
        const epPointerPtr = allocByteMem(pointerBytes);
        if (epPointerPtr === 0) {
          throw new Error("Could not allocate the opcode list pointer");
        }
        try {
          const bytesRaw = pluginInstance.exports["csound_opcode_init"](
            csoundInstance,
            epPointerPtr,
          );
          const bytes = Number(bytesRaw);
          const readPointer = (view, offset) =>
            pointerBytes === 8
              ? addressNumber(view.getBigUint64(offset, true))
              : view.getUint32(offset, true);
          const opcodeListPtr = readPointer(new DataView(memory.buffer), epPointerPtr);
          const entryCount = Math.floor(bytes / entrySize);
          if (entryCount > 0 && opcodeListPtr !== 0) {
            const allocationSize = entryCount * entrySize;
            if (!Number.isSafeInteger(allocationSize) || allocationSize > 0x7fffffff) {
              throw new RangeError("The opcode list is too large");
            }
            const patchedOpcodeListPtr = allocByteMem(allocationSize);
            if (patchedOpcodeListPtr === 0) {
              throw new Error("Could not allocate the opcode list");
            }
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
              return internFunction(table, funcRef);
            };

            try {
              for (let entryIndex = 0; entryIndex < entryCount; entryIndex += 1) {
                const sourceOffset = opcodeListPtr + entryIndex * entrySize;
                const targetOffset = patchedOpcodeListPtr + entryIndex * entrySize;

                new Uint8Array(memory.buffer, targetOffset, entrySize).set(
                  new Uint8Array(memory.buffer, sourceOffset, entrySize),
                );
                for (const offset of functionOffsets) {
                  const address = remapToHostTable(readPointer(dv, sourceOffset + offset));
                  if (pointerBytes === 8)
                    dv.setBigUint64(targetOffset + offset, BigInt(address), true);
                  else dv.setUint32(targetOffset + offset, address, true);
                }
              }

              appendOpcodes(csoundInstance, patchedOpcodeListPtr, entryCount);
            } finally {
              freeByteMem(patchedOpcodeListPtr);
            }
          } else {
            console.error("Invalid opcode table returned by csound_opcode_init");
          }
        } finally {
          freeByteMem(epPointerPtr);
        }
      } else if (typeof pluginInstance.exports["csound_opcode_init"] === "function") {
        console.error(
          `Missing required host exports for opcode plugin loading: ${missing.join(", ")}`,
        );
      }

      if (typeof pluginInstance.exports["csound_fgen_init"] === "function") {
        console.warn("csound_fgen_init plugins are not supported by the current WASM loader path.");
      }
      return;
    }

    const csoundOpcodeInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundFgenInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);

    if (typeof pluginInstance.exports["csound_opcode_init"] === "function") {
      csoundOpcodeInit.value = internFunction(table, pluginInstance.exports["csound_opcode_init"]);
    }

    if (typeof pluginInstance.exports["csound_fgen_init"] === "function") {
      csoundFgenInit.value = internFunction(table, pluginInstance.exports["csound_fgen_init"]);
    }

    hostExports["csoundWasiLoadOpcodeLibrary"](csoundInstance, csoundFgenInit, csoundOpcodeInit);
  } else {
    console.error("Plugin doesn't export nececcary functions to quality as csound plugin.");
  }
};
