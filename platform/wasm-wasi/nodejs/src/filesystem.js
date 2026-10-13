const decoder = new TextDecoder("utf-8");

function clearArray(array) {
  while (array.length > 0) {
    array.pop();
  }
}

export const csoundWasiJsMessageCallback = ({ memory, streamBuffer, messagePort }) => (
  csound,
  attribute,
  length_,
  offset,
) => {
  const buf = new Uint8Array(memory.buffer, offset, length_);
  const string = decoder.decode(buf);
  const endsWithNewline = /\n$/g.test(string);
  const startsWithNewline = /^\n/g.test(string);
  const chunks = string.split("\n").filter((item) => item.length > 0);
  const printableChunks = [];

  if ((chunks.length === 0 && endsWithNewline) || startsWithNewline) {
    printableChunks.push(streamBuffer.join(""));
    clearArray(streamBuffer);
  }
  chunks.forEach((chunk, index) => {
    // if it's last chunk
    if (index + 1 === chunks.length) {
      if (endsWithNewline) {
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
    console.log(chunk.replace(/(\r\n|\n|\r)/gm, ""));
    // if (messagePort.ready) {
    //   messagePort.post(chunk.replace(/(\r\n|\n|\r)/gm, ""));
    // }
  });
};
