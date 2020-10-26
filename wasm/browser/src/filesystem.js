import path from 'path';
import { cleanStdout, uint2String } from '@root/utils';
// import { isBrowser, isWebWorker } from 'browser-or-node';
// import { forEach, range } from 'ramda';
import { WasmFs } from '@wasmer/wasmfs';
import { Buffer } from 'buffer-es6';
// import { Buffer } from 'buffer';

export const touchFile = filename => {
  wasmFs.fs.writeFileSync(`/sandbox/${filename}`, '');
};

export const wasmFs = new WasmFs();

// wasmFs.fs.mkdirpSync('/sandbox', { mode: '0o777' });
// wasmFs.volume.mkdirpSync('/sandbox', { mode: '0o777' });

// if (isBrowser || isWebWorker) {
//   forEach(i => touchFile(i), range(0, 255));
// }

// --dir <wasm path>:<host path>
export const preopens = {
  '/': '/',
};

export const workerMessagePort = {
  ready: false,
  post: () => {},
  broadcastPlayState: () => {},
  vanillaWorkerState: undefined,
};

let stdErrorPos = 0;
const stdErrorBuffer = [];

let stdOutPos = 0;
const stdOutBuffer = [];

const stdErrorCallback = data => {
  const cleanString = cleanStdout(uint2String(data));
  if (cleanString.includes('\n')) {
    const [firstElement, ...next] = cleanString.split('\n');
    let outstr = '';
    while (stdErrorBuffer.length > 0) {
      outstr += stdErrorBuffer[0];
      stdErrorBuffer.shift();
    }

    outstr += firstElement || '';

    if (outstr && workerMessagePort.ready) {
      workerMessagePort.post(outstr);
    }

    next.forEach(s => stdErrorBuffer.push(s));
  } else {
    stdErrorBuffer.push(cleanString);
  }
};

const createStdErrorStream = () => {
  wasmFs.fs.watch('/dev/stderr', { encoding: 'buffer' }, (eventType, filename) => {
    if (filename) {
      const contents = wasmFs.fs.readFileSync('/dev/stderr');
      stdErrorCallback(contents.slice(stdErrorPos));
      stdErrorPos = contents.length;
    }
  });
};

const stdOutCallback = data => {
  const cleanString = cleanStdout(uint2String(data));
  if (cleanString.includes('\n')) {
    const [firstElement, ...next] = cleanString.split('\n');
    let outstr = '';
    while (stdOutBuffer.length > 0) {
      outstr += stdOutBuffer[0];
      stdOutBuffer.shift();
    }

    outstr += firstElement;
    if (outstr && workerMessagePort.ready) {
      workerMessagePort.post(outstr);
    }

    next.forEach(s => stdOutBuffer.push(s));
  } else {
    stdOutBuffer.push(cleanString);
  }
};

export const createStdOutStream = () => {
  wasmFs.fs.watch('/dev/stdout', { encoding: 'buffer' }, (eventType, filename) => {
    if (filename) {
      const contents = wasmFs.fs.readFileSync('/dev/stdout');
      stdOutCallback(contents.slice(stdOutPos));
      stdOutPos = contents.length;
    }
  });
};

export async function copyToFs(arrayBuffer, filePath) {
  const realPath = path.join('/sandbox', filePath);
  const buf = Buffer.from(new Uint8Array(arrayBuffer));
  wasmFs.fs.writeFileSync(realPath, buf);
}

export async function readFromFs(filePath) {
  const realPath = path.join('/sandbox', filePath);
  return wasmFs.fs.readFileSync(realPath);
}

export async function lsFs(lsPath) {
  const realPath = lsPath ? path.join('/sandbox', lsPath) : '/sandbox';
  return wasmFs.fs.readdirSync(realPath);
}

export async function llFs(llPath) {
  const realPath = llPath ? path.join('/sandbox', llPath) : '/sandbox';
  const files = wasmFs.fs.readdirSync(realPath);
  return files.map(file => ({ name: file, stat: wasmFs.fs.statSync(path.join(realPath, file)) }));
}

function rmrfFsRec(rmrfPath) {
  let rmrfPathSandboxed;
  if (typeof rmrfPath === 'string') {
    if (rmrfPath.startsWith('/sandbox')) {
      rmrfPathSandboxed = rmrfPath;
    } else {
      rmrfPathSandboxed = path.join('/sandbox', rmrfPath);
    }
  } else {
    rmrfPathSandboxed = '/sandbox';
  }
  if (wasmFs.fs.existsSync(rmrfPathSandboxed)) {
    wasmFs.fs.readdirSync(rmrfPathSandboxed).forEach(file => {
      var currentPath = path.join(rmrfPathSandboxed, file);
      if (wasmFs.fs.lstatSync(currentPath).isDirectory()) {
        rmrfFsRec(currentPath);
       } else {
        wasmFs.fs.unlinkSync(currentPath);
      }
    });
    wasmFs.fs.rmdirSync(rmrfPathSandboxed);
  }
}

export async function rmrfFs(rmrfPath) {
  rmrfFsRec(rmrfPath);
  wasmFs.volume.mkdirSync('/sandbox');
}

// all folders are stored under /csound, it seems as if
// sanboxing security increases, we are safer to have all assets
// nested from 1 and same root,
// this implementation is hidden from the Csound runtime itself with a hack
export async function mkdirp(filePath) {
  wasmFs.volume.mkdirpSync(path.join('/', filePath), {
    mode: '0o777',
  });
}

export const initFS = async wasm => {
  createStdErrorStream();
  createStdOutStream();
};
