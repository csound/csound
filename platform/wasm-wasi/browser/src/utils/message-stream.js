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

import { clearArray } from "./clear-array.js";

// Csound may send a heading and its columns in separate callback calls.
// Keep one buffer per worker and emit only complete lines.
export function bufferMessageText(string, messagePort, streamBuffer) {
  // CR ends a progress update even when no newline follows. Treat CRLF as
  // one ending; filtering empty chunks also handles it across two writes.
  const endsWithLineBreak = /[\r\n]$/.test(string);
  const startsWithLineBreak = /^[\r\n]/.test(string);
  const chunks = string.split(/\r\n|\r|\n/).filter((item) => item.length > 0);
  const printableChunks = [];
  if ((chunks.length === 0 && endsWithLineBreak) || startsWithLineBreak) {
    printableChunks.push(streamBuffer.join(""));
    clearArray(streamBuffer);
  }
  chunks.forEach((chunk, index) => {
    // if it's last chunk
    if (index + 1 === chunks.length) {
      if (endsWithLineBreak) {
        if (index === 0) {
          printableChunks.push(streamBuffer.join("") + chunk);
          clearArray(streamBuffer);
        } else {
          printableChunks.push(chunk);
        }
      } else {
        streamBuffer.push(chunk);
      }
    } else if (index === 0) {
      printableChunks.push(streamBuffer.join("") + chunk);
      clearArray(streamBuffer);
    } else {
      printableChunks.push(chunk);
    }
  });
  printableChunks.forEach((chunk) => {
    const maybePrintable = chunk.replaceAll(/(\r\n|\n|\r)/gm, "");
    if (maybePrintable) {
      messagePort.post({ log: chunk });
    }
  });
}
