import { encoder, decoder } from "../utils/text-encoders.js";
import * as constants from "./constants.js";

const googPath = goog.require("goog.string.path");

/** @define {boolean} */
const DEBUG_WASI = goog.define("DEBUG_WASI", false);

function removeLeadingSlash(path) {
  return path.replace(/^\//g, "");
}

function splitPathSegments(path) {
  if (!path) {
    return [];
  }
  return path.split("/").filter((segment) => segment.length > 0 && segment !== ".");
}

function normalizeAbsolutePath(path) {
  if (!path) {
    return "/";
  }
  const segments = splitPathSegments(path);
  const resolved = [];
  segments.forEach((segment) => {
    if (segment === "..") {
      if (resolved.length > 0) {
        resolved.pop();
      }
    } else {
      resolved.push(segment);
    }
  });
  return resolved.length > 0 ? `/${resolved.join("/")}` : "/";
}

function ensureAbsolutePath(basePath, path) {
  if (!path || path === ".") {
    return normalizeAbsolutePath(basePath || "/");
  }
  if (/^\//.test(path)) {
    return normalizeAbsolutePath(path);
  }
  const baseSegments = splitPathSegments(basePath || "/");
  const relativeSegments = path.split("/");
  const resolvedSegments = [...baseSegments];
  relativeSegments.forEach((segment) => {
    if (!segment || segment === ".") {
      return;
    }
    if (segment === "..") {
      if (resolvedSegments.length > 0) {
        resolvedSegments.pop();
      }
      return;
    }
    resolvedSegments.push(segment);
  });
  return resolvedSegments.length > 0 ? `/${resolvedSegments.join("/")}` : "/";
}

function shouldOpenReader(rights) {
  /** @suppress {checkTypes} */
  const bor = constants.WASI_RIGHT_FD_READ | constants.WASI_RIGHT_FD_READDIR;
  /** @suppress {suspiciousCode} */
  const result = (rights & bor) !== goog.global.BigInt(0);
  return result;
}

function performanceNowPoly() {
  /* eslint-disable-next-line unicorn/no-typeof-undefined */
  if (typeof performance === "undefined" || typeof performance.now === "undefined") {
    const nowOffset = Date.now();
    return Date.now() - nowOffset;
  } else {
    return performance.now();
  }
}

function concatUint8Arrays(arrays) {
  // sum of individual array lengths
  const totalLength = arrays.reduce((accumulator, value) => accumulator + value.length, 0);

  if (arrays.length === 0) return;

  const result = new Uint8Array(totalLength);

  // for each array - copy it over result
  // next array is copied right after the previous one
  let length = 0;
  for (const array of arrays) {
    result.set(array, length);
    length += array.length;
  }

  return result;
}

/**
 * @constructor
 * @this {WasiThis}
 */
export const WASI = function ({ preopens }) {
  this.fd = Array.from({ length: 4 });

  this.fd[0] = { fd: 0, path: "/dev/stdin", seekPos: goog.global.BigInt(0), buffers: [] };
  this.fd[1] = { fd: 1, path: "/dev/stdout", seekPos: goog.global.BigInt(0), buffers: [] };
  this.fd[2] = { fd: 2, path: "/dev/stderr", seekPos: goog.global.BigInt(0), buffers: [] };
  this.fd[3] = { fd: 3, path: "/", seekPos: goog.global.BigInt(0), buffers: [], type: "dir" };

  this.getMemory = this.getMemory.bind(this);
  this.CPUTIME_START = 0;
  this.cwd = "/";
  this.preopens = preopens || {};
};

/**
 * @function
 * @param {!WasmInst} instance
 */
WASI.prototype.start = function (instance) {
  this.CPUTIME_START = performanceNowPoly();
  const exports = instance["exports"];
  exports["_start"]();
};

/**
 * @function
 * @param {!WebAssembly.Module} module
 */
WASI.prototype.getImports = function (module) {
  const options = {};
  const neededImports = WebAssembly.Module.imports(module);

  for (const neededImport of neededImports) {
    if (neededImport["kind"] === "function" && neededImport.module.startsWith("wasi_")) {
      if (typeof options[neededImport["module"]] !== "object") {
        options[neededImport["module"]] = {};
      }
      options[neededImport["module"]][neededImport["name"]] = this[neededImport["name"]].bind(this);
    }
  }

  return options;
};

/**
 * @function
 * @param {!WebAssembly.Memory} memory
 */
WASI.prototype.setMemory = function (memory) {
  this.memory = memory;
};

/**
 * @function
 * @return {DataView}
 */
WASI.prototype.getMemory = function () {
  if (!this.view || !this.view.buffer || !this.view.buffer.byteLength) {
    this.view = new DataView(this.memory.buffer);
  }
  return this.view;
};

/**
 * @function
 * @param {string} path
 * @return {string}
 */
WASI.prototype.resolvePath = function (path) {
  return ensureAbsolutePath(this.cwd, path);
};

/**
 * @function
 * @param {string} filePath
 * @return {?Object}
 */
WASI.prototype.findEntry = function (filePath) {
  const normalized = normalizeAbsolutePath(filePath);
  const entries = Object.values(this.fd);
  for (const entry of entries) {
    if (entry && entry.path === normalized) {
      return entry;
    }
  }
  // eslint-disable-next-line unicorn/no-null
  return null;
};

/**
 * @function
 * @param {string} path
 * @return {number}
 */
WASI.prototype.chdir = function (path) {
  const targetPath = normalizeAbsolutePath(ensureAbsolutePath(this.cwd, path));

  if (targetPath === "/") {
    this.cwd = "/";
    if (this.fd[3]) {
      this.fd[3].path = "/";
      this.fd[3].type = "dir";
    }
    return constants.WASI_ESUCCESS;
  }

  const entry = this.findEntry(targetPath);

  if (!entry) {
    if (DEBUG_WASI) {
      console.warn(`chdir: path ${targetPath} does not exist`);
    }
    return constants.WASI_ENOENT;
  }

  if (entry.type && entry.type !== "dir") {
    if (DEBUG_WASI) {
      console.warn(`chdir: path ${targetPath} not a directory`);
    }
    return constants.WASI_ENOTDIR;
  }

  this.cwd = targetPath;
  if (this.fd[3]) {
    this.fd[3].path = targetPath;
    this.fd[3].type = "dir";
  }

  return constants.WASI_ESUCCESS;
};

WASI.prototype.msToNs = function (ms) {
  const msInt = Math.trunc(ms);
  const decimal = goog.global.BigInt(Math.round((ms - msInt) * 1000000));
  const ns = goog.global.BigInt(msInt) * goog.global.BigInt(1000000);
  return ns + decimal;
};

WASI.prototype.now = function (clockId) {
  switch (clockId) {
    case constants.WASI_CLOCK_MONOTONIC: {
      return Math.floor(performanceNowPoly());
    }
    case constants.WASI_CLOCK_REALTIME: {
      return this.msToNs(Date.now());
    }
    case constants.WASI_CLOCK_PROCESS_CPUTIME_ID:
    case constants.WASI_CLOCK_THREAD_CPUTIME_ID: {
      return Math.floor(performanceNowPoly() - this.CPUTIME_START);
    }
    default: {
      return 0;
    }
  }
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.args_get = function (argv, argvBuf) {
  if (DEBUG_WASI) {
    console.log("args_get", argv, argvBuf, constants);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.args_sizes_get = function (argc, argvBufSize) {
  if (DEBUG_WASI) {
    console.log("args_sizes_get", argc, argvBufSize, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.clock_res_get = function (clockId, resolution) {
  if (DEBUG_WASI) {
    console.log("args_get", clockId, resolution, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.clock_time_get = function (clockId, precision, time) {
  if (DEBUG_WASI) {
    console.log("clock_time_get", clockId, precision, time, arguments);
  }
  const memory = this.getMemory();
  const nextTime = this.now(clockId);
  memory.setBigUint64(time, goog.global.BigInt(nextTime), true);
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.environ_get = function (environ, environBuf) {
  if (DEBUG_WASI) {
    console.log("environ_get", environ, environBuf, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.environ_sizes_get = function (environCount, environBufSize) {
  if (DEBUG_WASI) {
    console.log("environ_sizes_get", environCount, environBufSize, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_advise = function (fd, offset, length_, advice) {
  if (DEBUG_WASI) {
    console.log("fd_advise", fd, offset, length_, advice, arguments);
  }
  return constants.WASI_ENOSYS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_allocate = function (fd, offset, length_) {
  if (DEBUG_WASI) {
    console.log("fd_allocate", fd, offset, length_, arguments);
  }
  return constants.WASI_ENOSYS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_close = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_close", fd, arguments);
  }

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_datasync = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_datasync", fd, arguments);
  }
  return constants.WASI_ESUCCESS;
};

// always write access in browser scope
/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_fdstat_get = function (fd, bufPtr) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_get", fd, bufPtr, arguments);
  }

  const memory = this.getMemory();

  memory.setUint8(bufPtr + 4, constants.WASI_FILETYPE_REGULAR_FILE);
  memory.setUint16(bufPtr + 2, 0, true);
  memory.setUint16(bufPtr + 4, 0, true);
  memory.setBigUint64(bufPtr + 8, goog.global.BigInt(constants.RIGHTS_REGULAR_FILE_BASE), true);
  memory.setBigUint64(
    bufPtr + 8 + 8,
    goog.global.BigInt(constants.RIGHTS_REGULAR_FILE_INHERITING),
    true,
  );

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_fdstat_set_flags = function (fd, flags) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_set_flags", fd, flags, arguments);
  }
  return constants.WASI_ENOSYS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_fdstat_set_rights = function (fd, fsRightsBase, fsRightsInheriting) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_set_rights", fd, fsRightsBase, fsRightsInheriting, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_filestat_get = function (fd, bufPtr) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_get", fd, bufPtr, arguments);
  }
  let filesize = 0;

  if (this.fd[fd]) {
    filesize = this.fd[fd].buffers.reduce(function (accumulator, uintArray) {
      return accumulator + uintArray?.byteLength ? uintArray?.byteLength : 0;
    }, 0);
  }

  const memory = this.getMemory();
  memory.setBigUint64(bufPtr, goog.global.BigInt(fd), true);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, goog.global.BigInt(fd), true);
  bufPtr += 8;
  memory.setUint8(bufPtr, constants.WASI_FILETYPE_REGULAR_FILE);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, goog.global.BigInt(1), true);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, goog.global.BigInt(filesize), true);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, this.msToNs(this.CPUTIME_START), true);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, this.msToNs(this.CPUTIME_START), true);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, this.msToNs(this.CPUTIME_START), true);

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_filestat_set_size = function (fd, newSize) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_set_size", fd, newSize, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_filestat_set_times = function (fd, stAtim, stMtim, filestatFags) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_set_times", fd, stAtim, stMtim, filestatFags, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_pread = function (fd, iovs, iovsLength, offset, nread) {
  if (DEBUG_WASI) {
    console.log("fd_pread", fd, iovs, iovsLength, offset, nread, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_prestat_dir_name = function (fd, pathPtr, pathLength) {
  if (DEBUG_WASI) {
    console.log("fd_prestat_dir_name", fd, pathPtr, pathLength, this.fd[fd]);
  }
  if (!this.fd[fd] && !this.fd[fd - 1]) {
    return constants.WASI_EBADF;
  }

  const { path: directoryName } = this.fd[fd];

  const memory = this.getMemory();

  const directoryNameBuffer = encoder.encode(directoryName);
  new Uint8Array(memory.buffer).set(directoryNameBuffer, pathPtr);

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_prestat_get = function (fd, bufPtr) {
  if (DEBUG_WASI) {
    console.log("fd_prestat_get", fd, bufPtr, this.fd[fd]);
  }
  if (!this.fd[fd]) {
    return constants.WASI_EBADF;
  }
  const { path: directoryName } = this.fd[fd];
  const memory = this.getMemory();

  const directoryNameBuffer = encoder.encode(directoryName);
  memory.setUint8(bufPtr, constants.WASI_PREOPENTYPE_DIR);
  memory.setUint32(bufPtr + 4, directoryNameBuffer.byteLength, true);
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_pwrite = function (fd, iovs, iovsLength, offset, nwritten) {
  console.log("fd_pwrite", fd, iovs, iovsLength, offset, nwritten, arguments);
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {(number | undefined)}
 */
WASI.prototype.fd_read = function (fd, iovs, iovsLength, nread) {
  if (DEBUG_WASI) {
    console.log("fd_read", fd, iovs, iovsLength, nread, arguments);
  }

  const memory = this.getMemory();
  const entry = this.fd[fd];

  if (!entry || !Array.isArray(entry.buffers)) {
    if (DEBUG_WASI) {
      console.error("fd_read: non-existent file descriptor", fd, entry);
    }
    memory.setUint32(nread, 0, true);
    return constants.WASI_EBADF;
  }

  const buffers = entry.buffers;

  if (buffers.length === 0) {
    memory.setUint32(nread, 0, true);
    entry.seekPos = goog.global.BigInt(0);
    return constants.WASI_ESUCCESS;
  }

  const totalBuffersLength = buffers.reduce((accumulator, b) => accumulator + b.length, 0);

  let read = Number(entry.seekPos);

  let thisRead = 0;
  let reduced = false;

  // check for EOF
  if (read >= totalBuffersLength) {
    const buf = memory.getUint32(iovs, true);
    memory.setUint8(buf, 0);
    memory.setUint32(nread, 0, true);
    return constants.WASI_ESUCCESS;
  }

  for (let index = 0; index < iovsLength; index++) {
    const ptr = iovs + index * 8;
    const buf = memory.getUint32(ptr, true);
    const bufLength = memory.getUint32(ptr + 4, true);

    if (!reduced) {
      thisRead += bufLength;
      Array.from({ length: bufLength }, (_, index) => index).reduce(
        (accumulator, currentRead) => {
          if (reduced) {
            return accumulator;
          }
          const [chunkIndex, chunkOffset] = accumulator;
          let currentChunkIndex = 0;
          let currentChunkOffset = 0;

          let found = false;
          let leadup = 0;

          let currentBufferChunkLength = buffers[currentChunkIndex]
            ? buffers[currentChunkIndex].byteLength
            : 0;

          if (currentRead === 0) {
            while (!found) {
              currentBufferChunkLength = buffers[currentChunkIndex]
                ? buffers[currentChunkIndex].byteLength
                : 0;
              if (leadup <= read && currentBufferChunkLength + leadup > read) {
                found = true;
                currentChunkOffset = read - leadup;
              } else {
                leadup += currentBufferChunkLength;
                currentChunkIndex += 1;
              }
            }
          } else {
            currentChunkIndex = chunkIndex;
            currentChunkOffset = chunkOffset;
          }

          if (buffers[currentChunkIndex]) {
            memory.setUint8(buf + currentRead, buffers[currentChunkIndex][currentChunkOffset]);

            if (currentChunkOffset + 1 >= buffers[currentChunkIndex].byteLength) {
              currentChunkIndex = chunkIndex + 1;
              currentChunkOffset = 0;
            } else {
              currentChunkOffset += 1;
            }
          } else {
            memory.setUint8(buf + currentRead, 0);
            read += currentRead;
            reduced = true;
          }

          return [currentChunkIndex, currentChunkOffset];
        },
        [0, 0],
      );
      if (!reduced) {
        read += bufLength;
      }
    }
  }

  entry.seekPos = goog.global.BigInt(read);
  memory.setUint32(nread, thisRead, true);

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_readdir = function (fd, bufPtr, bufLength, cookie, bufusedPtr) {
  if (DEBUG_WASI) {
    console.log("fd_readdir", fd, bufPtr, bufLength, cookie, bufusedPtr, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_renumber = function (from, to) {
  if (DEBUG_WASI) {
    console.log("fd_renumber", from, to, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_seek = function (fd, offset, whence, newOffsetPtr) {
  if (DEBUG_WASI) {
    console.log("fd_seek", fd, offset, whence, newOffsetPtr, arguments);
  }
  const memory = this.getMemory();

  switch (whence) {
    case constants.WASI_WHENCE_CUR: {
      /** @suppress {checkTypes} */
      this.fd[fd].seekPos =
        (this.fd[fd].seekPos ?? goog.global.BigInt(0)) + goog.global.BigInt(offset);
      break;
    }
    case constants.WASI_WHENCE_END: {
      const currentLength = (this.fd[fd].buffers || []).reduce(
        (accumulator, value) => accumulator + value.length,
        0,
      );
      this.fd[fd].seekPos = BigInt(currentLength) + BigInt(offset);
      break;
    }

    case constants.WASI_WHENCE_SET: {
      this.fd[fd].seekPos = BigInt(offset);
      break;
    }
  }

  memory.setBigUint64(newOffsetPtr, this.fd[fd].seekPos, true);

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_sync = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_sync", fd, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_tell = function (fd, offsetPtr) {
  if (DEBUG_WASI) {
    console.log("fd_tell", fd, offsetPtr, arguments);
  }
  const memory = this.getMemory();

  if (!this.fd[fd].seekPos) {
    this.fd[fd].seekPos = goog.global.BigInt(0);
  }

  memory.setBigUint64(offsetPtr, this.fd[fd].seekPos, true);

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.fd_write = function (fd, iovs, iovsLength, nwritten) {
  if (DEBUG_WASI) {
    console.log("fd_write", { fd, iovs, iovsLength, nwritten });
  }

  let append = false;
  const memory = this.getMemory();
  this.fd[fd].buffers = this.fd[fd].buffers || [];

  // append-only, if starting new write from beginning
  if (this.fd[fd].seekPos === goog.global.BigInt(0) && this.fd[fd].buffers.length > 0) {
    append = true;
  }
  let written = 0;

  for (let index = 0; index < iovsLength; index++) {
    const ptr = iovs + index * 8;
    const buf = memory.getUint32(ptr, true);
    const bufLength = memory.getUint32(ptr + 4, true);
    written += bufLength;
    const chunk = new Uint8Array(memory.buffer, buf, bufLength);
    if (append) {
      this.fd[fd].buffers.unshift(chunk.slice(0, bufLength));
    } else {
      this.fd[fd].buffers.push(chunk.slice(0, bufLength));
    }
  }

  /** @suppress {checkTypes} */
  this.fd[fd].seekPos += goog.global.BigInt(written);

  memory.setUint32(nwritten, written, true);

  if ([1, 2].includes(fd)) {
    console.log(decoder.decode(concatUint8Arrays(this.fd[fd].buffers)));
  }

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_create_directory = function (fd, pathPtr, pathLength) {
  if (DEBUG_WASI) {
    console.log("path_create_directory", fd, pathPtr, pathLength, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_filestat_get = function (fd, flags, pathPtr, pathLength, bufPtr) {
  if (DEBUG_WASI) {
    console.log("path_filestat_get", fd, flags, pathPtr, pathLength, bufPtr, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_filestat_set_times = function (
  fd,
  dirflags,
  pathPtr,
  pathLength,
  stAtim,
  stMtim,
  fstflags,
) {
  if (DEBUG_WASI) {
    console.log(
      "path_filestat_set_times",
      fd,
      dirflags,
      pathPtr,
      pathLength,
      stAtim,
      stMtim,
      fstflags,
      arguments,
    );
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_link = function (
  oldFd,
  oldFlags,
  oldPath,
  oldPathLength,
  newFd,
  newPath,
  newPathLength,
) {
  if (DEBUG_WASI) {
    console.log(
      "path_link",
      oldFd,
      oldFlags,
      oldPath,
      oldPathLength,
      newFd,
      newPath,
      newPathLength,
      arguments,
    );
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_open = function (
  dirfd,
  dirflags,
  pathPtr,
  pathLength,
  oflags,
  fsRightsBase,
  fsRightsInheriting,
  fsFlags,
  fd,
) {
  if (DEBUG_WASI) {
    console.log(
      "path_open",
      dirfd,
      dirflags,
      pathPtr,
      pathLength,
      oflags,
      fsRightsBase,
      fsRightsInheriting,
      fsFlags,
      fd,
      arguments,
    );
  }
  const memory = this.getMemory();
  const directoryPath = (this.fd[dirfd] || { path: this.cwd }).path;
  const pathOpenBytes = new Uint8Array(memory.buffer, pathPtr, pathLength);
  const pathOpenString = decoder.decode(pathOpenBytes);

  let pathOpen;
  if (dirfd === 3) {
    // Opening relative to the preopen root (cwd)
    pathOpen = this.resolvePath(pathOpenString);
  } else {
    // Opening relative to a specific directory fd
    const joined = googPath.join(directoryPath, pathOpenString);
    pathOpen = normalizeAbsolutePath(joined);
  }

  if (pathOpen.startsWith("/..") || pathOpen === "/._" || pathOpen === "/.AppleDouble") {
    return constants.WASI_EBADF;
  }

  const wantsDirectory = (oflags & constants.WASI_O_DIRECTORY) !== 0;
  const allowCreate = (oflags & constants.WASI_O_CREAT) !== 0;
  const existingEntry = this.findEntry(pathOpen);

  if (DEBUG_WASI) {
    console.log(";; path_open:", pathOpen, "from dirfd", dirfd);
    console.log("  withReader:", shouldOpenReader(fsRightsBase));
    console.log("  oflags:", oflags.toString(16), "fsRightsBase:", fsRightsBase.toString());
    console.log("  allowCreate:", allowCreate, "wantsDirectory:", wantsDirectory);
    console.log("  existingEntry:", existingEntry ? "exists" : "does not exist");
  }

  if (existingEntry && existingEntry.type === "dir" && !wantsDirectory) {
    return constants.WASI_EISDIR;
  }

  if (!existingEntry && wantsDirectory) {
    return constants.WASI_ENOENT;
  }

  // Check if file doesn't exist and shouldn't be created
  if (!existingEntry && !allowCreate && !wantsDirectory) {
    // File doesn't exist - write invalid fd and return ENOENT
    if (DEBUG_WASI) {
      console.warn(`path_open: file not found: ${pathOpen}`);
    }
    // Write maximum unsigned 32-bit value (-1 as signed) to indicate bad fd
    memory.setUint32(fd, 0xffffffff, true);
    return constants.WASI_ENOENT;
  }

  const actualFd = existingEntry ? existingEntry.fd : this.fd.length;

  if (!existingEntry && this.fd[actualFd] === undefined) {
    this.fd[actualFd] = { fd: actualFd };
  }

  const entryTemplate = existingEntry || this.fd[actualFd] || { fd: actualFd };

  this.fd[actualFd] = {
    ...entryTemplate,
    fd: actualFd,
    path: pathOpen,
    type: wantsDirectory ? "dir" : entryTemplate.type || "file",
    seekPos: goog.global.BigInt(0),
    buffers: Array.isArray(entryTemplate.buffers) ? entryTemplate.buffers : [],
  };

  if ((oflags & constants.WASI_O_TRUNC) !== 0 && !wantsDirectory) {
    this.fd[actualFd].buffers.length = 0;
  }

  if (shouldOpenReader(fsRightsBase) && DEBUG_WASI) {
    console.log("should open a read handle for", pathOpen);
  }

  memory.setUint32(fd, actualFd, true);

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_readlink = function (fd, pathPtr, pathLength, buf, bufLength, bufused) {
  if (DEBUG_WASI) {
    console.log("path_readlink", fd, pathPtr, pathLength, buf, bufLength, bufused, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_remove_directory = function (fd, pathPtr, pathLength) {
  if (DEBUG_WASI) {
    console.log("path_remove_directory", fd, pathPtr, pathLength);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_rename = function (
  oldFd,
  oldPath,
  oldPathLength,
  newFd,
  newPath,
  newPathLength,
) {
  if (DEBUG_WASI) {
    console.log(
      "path_rename",
      oldFd,
      oldPath,
      oldPathLength,
      newFd,
      newPath,
      newPathLength,
      arguments,
    );
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_symlink = function (oldPath, oldPathLength, fd, newPath, newPathLength) {
  if (DEBUG_WASI) {
    console.log("path_symlink", oldPath, oldPathLength, fd, newPath, newPathLength, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.path_unlink_file = function (fd, pathPtr, pathLength) {
  if (fd > 3 && DEBUG_WASI) {
    console.log("path_unlink_file", fd, pathPtr, pathLength, arguments);
  }
  // actual file removal goes here

  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.poll_oneoff = function (sin, sout, nsubscriptions, nevents) {
  if (DEBUG_WASI) {
    console.log("poll_oneoff", sin, sout, nsubscriptions, nevents, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.proc_exit = function (rval) {
  if (DEBUG_WASI) {
    console.log("proc_exit", rval, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.proc_raise = function (sig) {
  if (DEBUG_WASI) {
    console.log("proc_raise", sig, arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.random_get = function (bufPtr, bufLength) {
  if (DEBUG_WASI) {
    console.log("random_get", bufPtr, bufLength);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.sched_yield = function () {
  if (DEBUG_WASI) {
    console.log("sched_yield", arguments);
  }
  return constants.WASI_ESUCCESS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.sock_recv = function () {
  if (DEBUG_WASI) {
    console.log("sock_recv", arguments);
  }
  return constants.WASI_ENOSYS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.sock_send = function () {
  if (DEBUG_WASI) {
    console.log("sock_send", arguments);
  }
  return constants.WASI_ENOSYS;
};

/**
 * @export
 * @return {number}
 */
WASI.prototype.sock_shutdown = function () {
  if (DEBUG_WASI) {
    console.log("sock_shutdown", arguments);
  }
  return constants.WASI_ENOSYS;
};

// helpers

WASI.prototype.findBuffers = function (filePath /* string */) {
  const maybeFd = Object.values(this.fd).find(({ path }) => path === filePath);
  return maybeFd && maybeFd.buffers;
};

// fs api

WASI.prototype.readdir = function (dirname /* string */) {
  const absoluteDir = this.resolvePath(dirname);
  const prefixPath = absoluteDir === "/" ? "/" : `${absoluteDir}/`;
  const files = [];
  Object.values(this.fd).forEach((entry) => {
    if (!entry || !entry.path) {
      return;
    }
    const { path } = entry;
    if (!path.startsWith(prefixPath)) {
      return;
    }
    const rest = path.slice(prefixPath.length);
    if (rest.length === 0) {
      return;
    }
    if (!/\//g.test(rest)) {
      files.push(path);
    }
  });
  return files.map((p) => removeLeadingSlash(p.replace(prefixPath, ""))).filter((p) => !!p);
};

WASI.prototype.writeFile = function (fname /* string */, data /* Uint8Array */) {
  const filePath = this.resolvePath(fname);

  const nextFd = Object.keys(this.fd).length;
  const maybeOldFd = Object.values(this.fd).find(({ path }) => path === filePath);

  this.fd[nextFd] = {
    fd: nextFd,
    path: filePath,
    seekPos: goog.global.BigInt(0),
    buffers: [data],
    type: "file",
  };

  if (maybeOldFd) {
    delete this.fd[maybeOldFd];
  }
};

WASI.prototype.appendFile = function (fname /* string */, data /* Uint8Array */) {
  const filePath = this.resolvePath(fname);

  const buffers = this.findBuffers(filePath);

  if (buffers) {
    buffers.push(data);
  } else {
    console.error(`Can't append to non-existing file ${fname}`);
  }
};

WASI.prototype.readFile = function (fname /* string */) {
  const filePath = this.resolvePath(fname);

  const buffers = this.findBuffers(filePath);

  if (buffers) {
    return concatUint8Arrays(buffers);
  }
};

WASI.prototype.readStdOut = function () {
  const maybeFd = Object.values(this.fd[0]);
  const buffers = (maybeFd && maybeFd.buffers) || [];
  return concatUint8Arrays(buffers);
};

WASI.prototype.unlink = function (fname /* string */) {
  const filePath = this.resolvePath(fname);
  const maybeFd = Object.values(this.fd).find(({ path }) => path === filePath);

  if (maybeFd) {
    delete this.fd[maybeFd.fd];
  } else {
    console.error(`While trying to unlink ${filePath}, path not found`);
  }
};

WASI.prototype.mkdir = function (dirname /* string */) {
  const cleanPath = this.resolvePath(dirname);
  const files = [];
  Object.values(this.fd).forEach(({ path }) => {
    return path.startsWith(cleanPath) && files.push(path);
  });

  const alreadyExist = files.length > 0;
  if (alreadyExist) {
    console.warn(`mkdir: path ${dirname} already exists`);
  } else {
    const nextFd = Object.keys(this.fd).length;
    this.fd[nextFd] = {
      fd: nextFd,
      path: cleanPath,
      type: "dir",
    };
  }
};

WASI.prototype.stat = function (fname /* string */) {
  const filePath = this.resolvePath(fname);
  const maybeFd = Object.values(this.fd).find(({ path }) => path === filePath);

  if (!maybeFd) {
    return undefined;
  }

  const buffers = maybeFd.buffers || [];
  const size = buffers.reduce((accumulator, buffer) => {
    return accumulator + (buffer?.byteLength || 0);
  }, 0);

  const isDirectory = maybeFd.type === "dir";

  return {
    dev: 0,
    ino: maybeFd.fd,
    mode: isDirectory ? 16877 : 33188, // 0o40755 for dir, 0o100644 for file
    nlink: 1,
    uid: 0,
    gid: 0,
    rdev: 0,
    size,
    blksize: 4096,
    blocks: Math.ceil(size / 512),
    atimeMs: this.CPUTIME_START,
    mtimeMs: this.CPUTIME_START,
    ctimeMs: this.CPUTIME_START,
    birthtimeMs: this.CPUTIME_START,
    atime: new Date(this.CPUTIME_START),
    mtime: new Date(this.CPUTIME_START),
    ctime: new Date(this.CPUTIME_START),
    birthtime: new Date(this.CPUTIME_START),
    isFile: !isDirectory,
    isDirectory,
    isBlockDevice: false,
    isCharacterDevice: false,
    isSymbolicLink: false,
    isFIFO: false,
    isSocket: false,
  };
};

WASI.prototype.pathExists = function (fname /* string */) {
  const filePath = this.resolvePath(fname);
  const maybeFd = Object.values(this.fd).find(({ path }) => path === filePath);
  return !!maybeFd;
};
