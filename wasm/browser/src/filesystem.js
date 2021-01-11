import path from "path";
import { cleanStdout, uint2String } from "@root/utils";
import { Buffer } from "buffer-es6";

export const touchFile = (wasmFs, filename) => {
  wasmFs.fs.writeFileSync(`/sandbox/${filename}`, "");
};

const stdErrorCallback = (workerMessagePort, streamState) => (data) => {
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

const stdOutCallback = (workerMessagePort, streamState) => (data) => {
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

export async function writeToFs(wasmFs) {
  return (_, arrayBuffer, filePath) => {
    const realPath = path.join("/sandbox", filePath);
    const buf = Buffer.from(new Uint8Array(arrayBuffer));
    wasmFs.fs.writeFileSync(realPath, buf);
  };
}

export async function readFromFs(wasmFs) {
  return (_, filePath) => {
    const realPath = path.join("/sandbox", filePath);
    return wasmFs.fs.readFileSync(realPath);
  };
}

export async function lsFs(wasmFs) {
  return (_, lsPath) => {
    const realPath = lsPath ? path.join("/sandbox", lsPath) : "/sandbox";
    return wasmFs.fs.readdirSync(realPath);
  };
}

export async function llFs(wasmFs) {
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
    if (rmrfPath.startsWith("/sandbox")) {
      rmrfPathSandboxed = rmrfPath;
    } else {
      rmrfPathSandboxed = path.join("/sandbox", rmrfPath);
    }
  } else {
    rmrfPathSandboxed = "/sandbox";
  }
  if (wasmFs.fs.existsSync(rmrfPathSandboxed)) {
    wasmFs.fs.readdirSync(rmrfPathSandboxed).forEach((file) => {
      var currentPath = path.join(rmrfPathSandboxed, file);
      if (wasmFs.fs.lstatSync(currentPath).isDirectory()) {
        rmrfFsRec(wasmFs, currentPath);
      } else {
        wasmFs.fs.unlinkSync(currentPath);
      }
    });
    wasmFs.fs.rmdirSync(rmrfPathSandboxed);
  }
}

export async function rmrfFs(wasmFs) {
  return (_, rmrfPath) => {
    rmrfFsRec(wasmFs, rmrfPath);
    wasmFs.volume.mkdirpSync("/sandbox");
  };
}

export async function mkdirp(wasmFs) {
  return (_, filePath) => {
    wasmFs.volume.mkdirpSync(path.join("/", filePath), {
      mode: "0o777",
    });
  };
}

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
