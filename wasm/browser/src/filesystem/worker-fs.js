import * as path from "path-browserify";
import { Buffer } from "buffer-es6";
import { uint2String } from "@utils/text-encoders";
import { cleanStdout } from "@utils/clean-stdout-string";
import { clearArray } from "@utils/clear-array";

export const touchFile = (wasmFs, filename) => {
  wasmFs.fs.writeFileSync(`/sandbox/${filename}`, "");
};

export const stdErrorCallback = (workerMessagePort, streamState) => (data) => {
  const cleanString = cleanStdout(uint2String(data));
  if (cleanString.includes("\n")) {
    const [firstElement, ...next] = cleanString.split("\n");
    let outstr = "";
    while (streamState.stdErrorBuffer.length > 0) {
      outstr += streamState.stdErrorBuffer[0];
      streamState.stdErrorBuffer.shift();
    }

    outstr += firstElement || "";
    if (outstr && workerMessagePort.ready) {
      workerMessagePort.post(outstr);
    }

    next.forEach((s) => streamState.stdErrorBuffer.push(s));
  } else {
    streamState.stdErrorBuffer.push(cleanString);
  }
};

const createStdErrorStream = (wasmFs, workerMessagePort, streamState) => {
  const stdErrorCallback_ = stdErrorCallback(workerMessagePort, streamState);
  const watcher = new wasmFs.volume.FSWatcher();
  function listener(eventType, filename) {
    if (filename) {
      const contents = wasmFs.fs.readFileSync("/dev/stderr");
      stdErrorCallback_(contents.slice(streamState.stdErrorPos));
      streamState.stdErrorPos = contents.length;
    }
  }
  watcher.start("/dev/stderr", true, false, "buffer");
  watcher.addListener("change", listener);
  return watcher;
};

export const stdOutCallback = (workerMessagePort, streamState) => (data) => {
  const cleanString = cleanStdout(uint2String(data));
  if (cleanString.includes("\n")) {
    const [firstElement, ...next] = cleanString.split("\n");
    let outstr = "";
    while (streamState.stdOutBuffer.length > 0) {
      outstr += streamState.stdOutBuffer[0];
      streamState.stdOutBuffer.shift();
    }

    outstr += firstElement;
    if (outstr && workerMessagePort.ready) {
      workerMessagePort.post(outstr);
    }

    next.forEach((s) => streamState.stdOutBuffer.push(s));
  } else {
    streamState.stdOutBuffer.push(cleanString);
  }
};

export const createStdOutStream = (wasmFs, workerMessagePort, streamState) => {
  const stdOutCallback_ = stdOutCallback(workerMessagePort, streamState);
  const watcher = new wasmFs.volume.FSWatcher();
  function listener(eventType, filename) {
    if (filename) {
      const contents = wasmFs.fs.readFileSync("/dev/stdout");
      stdOutCallback_(contents.slice(streamState.stdOutPos));
      streamState.stdOutPos = contents.length;
    }
  }
  watcher.start("/dev/stdout", true, false, "buffer");
  watcher.addListener("change", listener);
  return watcher;
};

export const csoundWasiJsMessageCallback = ({ memory, streamBuffer, messagePort }) => (
  csound,
  attribute,
  length_,
  offset,
) => {
  const buf = new Uint8Array(memory.buffer, offset, length_);
  const string = uint2String(buf);
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
    if (messagePort.ready) {
      messagePort.post(chunk.replace(/(\r\n|\n|\r)/gm, ""));
    }
  });
};

export function writeToFs(wasmFs) {
  return (_, arrayBuffer, filePath) => {
    const realPath = path.join("/sandbox", filePath);
    const buf = Buffer.from(new Uint8Array(arrayBuffer));
    wasmFs.fs.writeFileSync(realPath, buf);
  };
}

export function readFromFs(wasmFs) {
  return (_, filePath) => {
    const realPath = path.join("/sandbox", filePath);
    return wasmFs.fs.readFileSync(realPath);
  };
}

export function lsFs(wasmFs) {
  return (_, lsPath) => {
    const realPath = lsPath ? path.join("/sandbox", lsPath) : "/sandbox";
    return wasmFs.fs.readdirSync(realPath);
  };
}

export function llFs(wasmFs) {
  return (_, llPath) => {
    const realPath = llPath ? path.join("/sandbox", llPath) : "/sandbox";
    const files = wasmFs.fs.readdirSync(realPath);
    return files.map((file) => ({
      name: file,
      stat: wasmFs.fs.statSync(path.join(realPath, file)),
    }));
  };
}

function rmrfFsRec(wasmFs, rmrfPath) {
  let rmrfPathSandboxed;
  if (typeof rmrfPath === "string") {
    rmrfPathSandboxed = rmrfPath.startsWith("/sandbox")
      ? rmrfPath
      : path.join("/sandbox", rmrfPath);
  } else {
    rmrfPathSandboxed = "/sandbox";
  }
  if (wasmFs.fs.existsSync(rmrfPathSandboxed)) {
    wasmFs.fs.readdirSync(rmrfPathSandboxed).forEach((file) => {
      const currentPath = path.join(rmrfPathSandboxed, file);
      if (wasmFs.fs.lstatSync(currentPath).isDirectory()) {
        rmrfFsRec(wasmFs, currentPath);
      } else {
        wasmFs.fs.unlinkSync(currentPath);
      }
    });
    wasmFs.fs.rmdirSync(rmrfPathSandboxed);
  }
}

export function rmrfFs(wasmFs) {
  return (_, rmrfPath) => {
    rmrfFsRec(wasmFs, rmrfPath);
    wasmFs.volume.mkdirpSync("/sandbox");
  };
}

export function mkdirp(wasmFs) {
  return (_, filePath) => {
    wasmFs.volume.mkdirpSync(path.join("/", filePath), {
      mode: "0o777",
    });
  };
}

export function getWorkerFs(wasmFs) {
  return () => wasmFs.toJSON("/sandbox");
}

// modified from @wasmer/wasmfs
function fromJSONFixed(memory, vol, json) {
  const seperator = "/";

  if (json.__unlink) {
    for (const doUnlink_ of json.__unlink) {
      const doUnlink = (doUnlink_.startsWith("/") ? "/sandbox" : "/sandbox/") + doUnlink_;
      vol.unlinkSync(doUnlink);
    }
    delete json.__unlink;
  }

  for (const filename_ in json) {
    const filename = (filename_.startsWith("/") ? "/sandbox" : "/sandbox/") + filename_;
    const data = json[filename_];
    const isDirectory = data ? !Object.getPrototypeOf(data) : !data;
    if (!isDirectory) {
      const steps = filename.split(seperator);
      if (steps.length > 1) {
        const dirname = seperator + steps.slice(0, -1).join(seperator);
        vol.mkdirpBase(dirname, 0o777);
      }
      vol.writeFileSync(filename, data ? Buffer.from(data) : data, { mode: 0o777 });
    } else {
      vol.mkdirpBase(filename, 0o777);
    }
  }
}

export const syncWorkerFs = (memory, wasmFs) => (_, persistentStorage) => {
  fromJSONFixed(memory, wasmFs.volume, persistentStorage);
};

export const initFS = (wasmFs, messagePort) => {
  const streamState = {
    stdErrorPos: 0,
    stdErrorBuffer: [],
    stdOutPos: 0,
    stdOutBuffer: [],
  };

  return [
    createStdOutStream(wasmFs, messagePort, streamState),
    createStdErrorStream(wasmFs, messagePort, streamState),
  ];
};
