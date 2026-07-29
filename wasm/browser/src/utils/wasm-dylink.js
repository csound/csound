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

const emptyHeader = () => ({
  hasDylink: false,
  sectionSize: 0,
  memorySize: 0,
  memoryAlign: 0,
  tableSize: 0,
  tableAlign: 0,
  neededDynlibsCount: 0,
  neededDynlibs: [],
});

const isWasmBinary = (wasmBytes) =>
  wasmBytes.length >= 8 &&
  wasmBytes[0] === 0x00 &&
  wasmBytes[1] === 0x61 &&
  wasmBytes[2] === 0x73 &&
  wasmBytes[3] === 0x6d;

export const getBinaryHeaderData = (
  input,
  nameDecoder = new TextDecoder("utf8"),
) => {
  const wasmBytes = input instanceof Uint8Array ? input : new Uint8Array(input);
  if (!isWasmBinary(wasmBytes)) {
    return emptyHeader();
  }

  let position = 8;

  const readULEB = (limit) => {
    let result = 0;
    let multiplier = 1;

    for (let byteIndex = 0; byteIndex < 5; byteIndex += 1) {
      if (position >= limit) {
        throw new Error("Unexpected end of WebAssembly data while reading ULEB");
      }

      const byte = wasmBytes[position];
      position += 1;
      result += (byte & 0x7f) * multiplier;

      if (result > 0xffffffff) {
        throw new Error("WebAssembly ULEB value exceeds uint32");
      }
      if ((byte & 0x80) === 0) {
        return result;
      }

      multiplier *= 128;
    }

    throw new Error("WebAssembly ULEB value is too long");
  };

  const readName = (limit) => {
    const length = readULEB(limit);
    if (position + length > limit) {
      throw new Error("Unexpected end of WebAssembly data while reading a name");
    }
    const name = nameDecoder.decode(wasmBytes.subarray(position, position + length));
    position += length;
    return name;
  };

  const parseLegacyDylink = (sectionSize, sectionEnd) => {
    const memorySize = readULEB(sectionEnd);
    const memoryAlign = readULEB(sectionEnd);
    const tableSize = readULEB(sectionEnd);
    const tableAlign = readULEB(sectionEnd);
    const neededDynlibsCount = readULEB(sectionEnd);
    const neededDynlibs = [];

    for (let index = 0; index < neededDynlibsCount; index += 1) {
      neededDynlibs.push(readName(sectionEnd));
    }
    if (position !== sectionEnd) {
      throw new Error("Unexpected data at the end of the dylink section");
    }

    return {
      hasDylink: true,
      sectionSize,
      memorySize,
      memoryAlign,
      tableSize,
      tableAlign,
      neededDynlibsCount,
      neededDynlibs,
    };
  };

  const parseDylink0 = (sectionSize, sectionEnd) => {
    const header = emptyHeader();
    header.hasDylink = true;
    header.sectionSize = sectionSize;
    let hasMemoryInfo = false;

    while (position < sectionEnd) {
      const subsectionType = wasmBytes[position];
      position += 1;
      const subsectionSize = readULEB(sectionEnd);
      const subsectionEnd = position + subsectionSize;
      if (subsectionEnd > sectionEnd) {
        throw new Error("dylink.0 subsection extends past its section");
      }

      if (subsectionType === 1) {
        if (hasMemoryInfo) {
          throw new Error("dylink.0 contains more than one memory info subsection");
        }
        hasMemoryInfo = true;
        header.memorySize = readULEB(subsectionEnd);
        header.memoryAlign = readULEB(subsectionEnd);
        header.tableSize = readULEB(subsectionEnd);
        header.tableAlign = readULEB(subsectionEnd);
        if (position !== subsectionEnd) {
          throw new Error("Unexpected data in the dylink.0 memory info subsection");
        }
      } else if (subsectionType === 2) {
        const neededCount = readULEB(subsectionEnd);
        for (let index = 0; index < neededCount; index += 1) {
          header.neededDynlibs.push(readName(subsectionEnd));
        }
        if (position !== subsectionEnd) {
          throw new Error("Unexpected data in the dylink.0 needed libraries subsection");
        }
      }

      position = subsectionEnd;
    }

    header.neededDynlibsCount = header.neededDynlibs.length;
    return header;
  };

  while (position < wasmBytes.length) {
    const sectionId = wasmBytes[position];
    position += 1;
    const sectionSize = readULEB(wasmBytes.length);
    const sectionEnd = position + sectionSize;
    if (sectionEnd > wasmBytes.length) {
      throw new Error("WebAssembly section extends past the end of the binary");
    }

    if (sectionId === 0) {
      const sectionName = readName(sectionEnd);
      if (sectionName === "dylink") {
        return parseLegacyDylink(sectionSize, sectionEnd);
      }
      if (sectionName === "dylink.0") {
        return parseDylink0(sectionSize, sectionEnd);
      }
    }

    position = sectionEnd;
  }

  return emptyHeader();
};
