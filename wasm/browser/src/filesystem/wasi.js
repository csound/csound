import { encoder, decoder } from "../utils/text-encoders.js";
import * as constants from "./constants.js";

const { normalizePath } = goog.require("goog.string.path");

/** @define {boolean} */
const DEBUG_WASI = goog.define("DEBUG_WASI", false);

function assertLeadingSlash(path) {
  return /^\//g.test(path) ? path : `/${path}`;
}

function removeLeadingSlash(path) {
  return path.replace(/^\//g, "");
}

function shouldOpenReader(rights) {
  return (
    (rights & (constants.WASI_RIGHT_FD_READ | constants.WASI_RIGHT_FD_READDIR)) !==
    goog.global.BigInt(0)
  );
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

export const WASI = function ({ preopens }) {
  this.fd = Array.from({ length: 4 });

  this.fd[0] = {
    fd: 0,
    path: "/dev/stdin",
    seekPos: goog.global.BigInt(0),
    buffers: [],
    open: false,
  };
  this.fd[1] = {
    fd: 1,
    path: "/dev/stdout",
    seekPos: goog.global.BigInt(0),
    buffers: [],
    open: false,
  };
  this.fd[2] = {
    fd: 2,
    path: "/dev/stderr",
    seekPos: goog.global.BigInt(0),
    buffers: [],
    open: false,
  };
  this.fd[3] = { fd: 3, path: "/", seekPos: goog.global.BigInt(0), buffers: [], open: false };

  this.getMemory = this.getMemory.bind(this);
  this.CPUTIME_START = 0;
};

/**
 * @function
 * @param {!WebAssembly.Instance} instance
 */
WASI.prototype.start = function (instance) {
  this.CPUTIME_START = performanceNowPoly();
  const exports = instance.exports;
  exports._start();
};

/**
 * @function
 * @param {!WebAssembly.Module} instance
 */
WASI.prototype.getImports = function (module) {
  const options = {};
  const neededImports = WebAssembly.Module.imports(module);

  for (const neededImport of neededImports) {
    if (neededImport.kind === "function" && neededImport.module.startsWith("wasi_")) {
      if (typeof options[neededImport.module] !== "object") {
        options[neededImport.module] = {};
      }
      options[neededImport.module][neededImport.name] = this[neededImport.name].bind(this);
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

WASI.prototype.args_get = function (argv, argvBuf) {
  if (DEBUG_WASI) {
    console.log("args_get", argv, argvBuf, constants);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.args_sizes_get = function (argc, argvBufSize) {
  if (DEBUG_WASI) {
    console.log("args_sizes_get", argc, argvBufSize, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.clock_res_get = function (clockId, resolution) {
  if (DEBUG_WASI) {
    console.log("args_get", clockId, resolution, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.clock_time_get = function (clockId, precision, time) {
  if (DEBUG_WASI) {
    console.log("clock_time_get", clockId, precision, time, arguments);
  }
  const memory = this.getMemory();
  const nextTime = this.now(clockId);
  memory.setBigUint64(time, goog.global.BigInt(nextTime), true);
  return constants.WASI_ESUCCESS;
};
WASI.prototype.environ_get = function (environ, environBuf) {
  if (DEBUG_WASI) {
    console.log("environ_get", environ, environBuf, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.environ_sizes_get = function (environCount, environBufSize) {
  if (DEBUG_WASI) {
    console.log("environ_sizes_get", environCount, environBufSize, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.fd_advise = function (fd, offset, length_, advice) {
  if (DEBUG_WASI) {
    console.log("fd_advise", fd, offset, length_, advice, arguments);
  }
  return constants.WASI_ENOSYS;
};
WASI.prototype.fd_allocate = function (fd, offset, length_) {
  if (DEBUG_WASI) {
    console.log("fd_allocate", fd, offset, length_, arguments);
  }
  return constants.WASI_ENOSYS;
};
WASI.prototype.fd_close = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_close", fd, arguments);
  }
  if (this.fd[fd]) {
    this.fd[fd].open = false;
  }

  return constants.WASI_ESUCCESS;
};
WASI.prototype.fd_datasync = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_datasync", fd, arguments);
  }
  return constants.WASI_ESUCCESS;
};

// always write access in browser scope
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

WASI.prototype.fd_fdstat_set_flags = function (fd, flags) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_set_flags", fd, flags, arguments);
  }
  return constants.WASI_ENOSYS;
};
WASI.prototype.fd_fdstat_set_rights = function (fd, fsRightsBase, fsRightsInheriting) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_set_rights", fd, fsRightsBase, fsRightsInheriting, arguments);
  }
  return constants.WASI_ESUCCESS;
};

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

WASI.prototype.fd_filestat_set_size = function (fd, newSize) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_set_size", fd, newSize, arguments);
  }
  return constants.WASI_ESUCCESS;
};

WASI.prototype.fd_filestat_set_times = function (fd, stAtim, stMtim, filestatFags) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_set_times", fd, stAtim, stMtim, filestatFags, arguments);
  }
  return constants.WASI_ESUCCESS;
};

WASI.prototype.fd_pread = function (fd, iovs, iovsLength, offset, nread) {
  if (DEBUG_WASI) {
    console.log("fd_pread", fd, iovs, iovsLength, offset, nread, arguments);
  }
  return constants.WASI_ESUCCESS;
};

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

WASI.prototype.fd_pwrite = function (fd, iovs, iovsLength, offset, nwritten) {
  console.log("fd_pwrite", fd, iovs, iovsLength, offset, nwritten, arguments);
  return constants.WASI_ESUCCESS;
};

WASI.prototype.fd_read = function (fd, iovs, iovsLength, nread) {
  if (DEBUG_WASI) {
    console.log("fd_read", fd, iovs, iovsLength, nread, arguments);
  }
  const buffers = this.fd[fd] && this.fd[fd].buffers;
  const totalBuffersLength = buffers.reduce((accumulator, b) => accumulator + b.length, 0);
  const memory = this.getMemory();

  if (!buffers || buffers.length === 0) {
    console.error("Reading non existent file", fd, this.fd[fd]);
    return;
  }

  let read = Number(this.fd[fd].seekPos);

  let thisRead = 0;
  let reduced = false;

  // check for EOF
  if (read >= totalBuffersLength) {
    const buf = memory.getUint32(iovs, true);
    memory.setUint8(buf, "\0");
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
            memory.setUint8(buf + currentRead, "\0");
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

  this.fd[fd].seekPos = goog.global.BigInt(read);
  memory.setUint32(nread, thisRead, true);

  return constants.WASI_ESUCCESS;
};

WASI.prototype.fd_readdir = function (fd, bufPtr, bufLength, cookie, bufusedPtr) {
  if (DEBUG_WASI) {
    console.log("fd_readdir", fd, bufPtr, bufLength, cookie, bufusedPtr, arguments);
  }
  return constants.WASI_ESUCCESS;
};

WASI.prototype.fd_renumber = function (from, to) {
  if (DEBUG_WASI) {
    console.log("fd_renumber", from, to, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.fd_seek = function (fd, offset, whence, newOffsetPtr) {
  if (DEBUG_WASI) {
    console.log("fd_seek", fd, offset, whence, newOffsetPtr, arguments);
  }
  const memory = this.getMemory();

  switch (whence) {
    case constants.WASI_WHENCE_CUR: {
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
WASI.prototype.fd_sync = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_sync", fd, arguments);
  }
  return constants.WASI_ESUCCESS;
};
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

WASI.prototype.fd_write = function (fd, iovs, iovsLength, nwritten) {
  if (DEBUG_WASI) {
    console.log("fd_write", { fd, iovs, iovsLength, nwritten });
  }

  const memory = this.getMemory();
  this.fd[fd].buffers = this.fd[fd].buffers || [];
  this.fd[fd].buffers =
    this.fd[fd].buffers.length > 0 ? [concatUint8Arrays(this.fd[fd].buffers)] : this.fd[fd].buffers;

  let written = 0;

  for (let index = 0; index < iovsLength; index++) {
    const ptr = iovs + index * 8;
    const buf = memory.getUint32(ptr, true);
    const bufLength = memory.getUint32(ptr + 4, true);
    written += bufLength;
    const chunk = new Uint8Array(memory.buffer, buf, bufLength);
    if (this.fd[fd].buffers[0] && this.fd[fd].seekPos < this.fd[fd].buffers[0].length) {
      const seekPosInt = Number(this.fd[fd].seekPos);
      chunk.slice(0, bufLength).forEach((b, i) => {
        this.fd[fd].buffers[0][seekPosInt + i] = b;
      });
    } else {
      this.fd[fd].buffers.push(chunk.slice(0, bufLength));
    }
  }

  this.fd[fd].seekPos += goog.global.BigInt(written);

  memory.setUint32(nwritten, written, true);

  if ([1, 2].includes(fd)) {
    console.log(decoder.decode(concatUint8Arrays(this.fd[fd].buffers)));
  }

  return constants.WASI_ESUCCESS;
};

WASI.prototype.path_create_directory = function (fd, pathPtr, pathLength) {
  if (DEBUG_WASI) {
    console.log("path_create_directory", fd, pathPtr, pathLength, arguments);
  }
  return constants.WASI_ESUCCESS;
};

WASI.prototype.path_filestat_get = function (fd, flags, pathPtr, pathLength, bufPtr) {
  if (DEBUG_WASI) {
    console.log("path_filestat_get", fd, flags, pathPtr, pathLength, bufPtr, arguments);
  }
  return constants.WASI_ESUCCESS;
};
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
  const directoryPath = (this.fd[dirfd] || { path: "/" }).path;
  const pathOpenBytes = new Uint8Array(memory.buffer, pathPtr, pathLength);
  const pathOpenString = decoder.decode(pathOpenBytes);
  const pathOpen = assertLeadingSlash(
    normalizePath(goog.string.path.join(dirfd === 3 ? "" : directoryPath, pathOpenString)),
  );

  if (DEBUG_WASI) {
    console.log(";; opening path", pathOpen, "withREader", shouldOpenReader(fsRightsBase));
  }

  if (pathOpen.startsWith("/..") || pathOpen === "/._" || pathOpen === "/.AppleDouble") {
    return constants.WASI_EBADF;
  }

  const alreadyExists = Object.values(this.fd).find(
    (entry) => entry.path === pathOpen && Array.isArray(entry.buffers),
  );
  let actualFd;

  if (alreadyExists) {
    actualFd = alreadyExists.fd;
  } else {
    actualFd = this.fd.length;
    this.fd[actualFd] = { fd: actualFd };
  }

  let fileType = "file";

  this.fd[actualFd] = {
    ...this.fd[actualFd],
    path: pathOpen,
    type: fileType,
    seekPos: goog.global.BigInt(0),
    buffers: alreadyExists ? this.fd[actualFd].buffers : [],
    open: true,
  };

  if ((oflags & constants.WASI_O_DIRECTORY) !== 0) {
    fileType = "dir";
  }

  if (shouldOpenReader(fsRightsBase) && DEBUG_WASI) {
    console.log("should open a read handle for", pathOpen);
  }

  memory.setUint32(fd, actualFd, true);

  return constants.WASI_ESUCCESS;
};

WASI.prototype.path_readlink = function (fd, pathPtr, pathLength, buf, bufLength, bufused) {
  if (DEBUG_WASI) {
    console.log("path_readlink", fd, pathPtr, pathLength, buf, bufLength, bufused, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.path_remove_directory = function (fd, pathPtr, pathLength) {
  if (DEBUG_WASI) {
    console.log("path_remove_directory", fd, pathPtr, pathLength);
  }
  return constants.WASI_ESUCCESS;
};
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
WASI.prototype.path_symlink = function (oldPath, oldPathLength, fd, newPath, newPathLength) {
  if (DEBUG_WASI) {
    console.log("path_symlink", oldPath, oldPathLength, fd, newPath, newPathLength, arguments);
  }
  return constants.WASI_ESUCCESS;
};

WASI.prototype.path_unlink_file = function (fd, pathPtr, pathLength) {
  if (fd > 3 && DEBUG_WASI) {
    console.log("path_unlink_file", fd, pathPtr, pathLength, arguments);
  }
  // actual file removal goes here

  return constants.WASI_ESUCCESS;
};

WASI.prototype.poll_oneoff = function (sin, sout, nsubscriptions, nevents) {
  if (DEBUG_WASI) {
    console.log("poll_oneoff", sin, sout, nsubscriptions, nevents, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.proc_exit = function (rval) {
  if (DEBUG_WASI) {
    console.log("proc_exit", rval, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.proc_raise = function (sig) {
  if (DEBUG_WASI) {
    console.log("proc_raise", sig, arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.random_get = function (bufPtr, bufLength) {
  if (DEBUG_WASI) {
    console.log("random_get", bufPtr, bufLength);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.sched_yield = function () {
  if (DEBUG_WASI) {
    console.log("sched_yield", arguments);
  }
  return constants.WASI_ESUCCESS;
};
WASI.prototype.sock_recv = function () {
  if (DEBUG_WASI) {
    console.log("sock_recv", arguments);
  }
  return constants.WASI_ENOSYS;
};
WASI.prototype.sock_send = function () {
  if (DEBUG_WASI) {
    console.log("sock_send", arguments);
  }
  return constants.WASI_ENOSYS;
};
WASI.prototype.sock_shutdown = function () {
  if (DEBUG_WASI) {
    console.log("sock_shutdown", arguments);
  }
  return constants.WASI_ENOSYS;
};

// helpers

WASI.prototype.findBuffers = function (filePath /* string */) {
  const maybeFd = Object.values(this.fd).find(({ path }) => path === filePath);
  return [maybeFd && maybeFd.buffers, maybeFd.fd];
};

// fs api

WASI.prototype.readdir = function (dirname /* string */) {
  const prefixPath = (assertLeadingSlash(normalizePath(dirname)) + "/").replace("//", "/");
  const files = [];
  Object.values(this.fd).forEach(({ path }) => {
    // console.log({
    //   path,
    //   prefixPath,
    //   replaced: path.replace(prefixPath, ""),
    //   isTrue: !/\//g.test(path.replace(prefixPath, "")),
    // });
    return !/\//g.test(path.replace(prefixPath, "")) && files.push(path);
  });
  return files.map((p) => removeLeadingSlash(p.replace(prefixPath, ""))).filter((p) => !!p);
};

WASI.prototype.writeFile = function (fname /* string */, data /* Uint8Array */) {
  const filePath = assertLeadingSlash(normalizePath(fname));

  const nextFd = Object.keys(this.fd).length;
  const maybeOldFd = Object.values(this.fd).find(({ path }) => path === filePath);

  this.fd[nextFd] = {
    fd: nextFd,
    path: filePath,
    seekPos: goog.global.BigInt(0),
    buffers: [data],
  };

  if (maybeOldFd) {
    delete this.fd[maybeOldFd];
  }
};

WASI.prototype.appendFile = function (fname /* string */, data /* Uint8Array */) {
  const filePath = assertLeadingSlash(normalizePath(fname));

  const [buffers] = this.findBuffers(filePath);

  if (buffers) {
    buffers.push(data);
  } else {
    console.error(`Can't append to non-existing file ${fname}`);
  }
};

WASI.prototype.readFile = function (fname /* string */) {
  const filePath = assertLeadingSlash(normalizePath(fname));

  const [buffers, fd] = this.findBuffers(filePath);
  if (this.fd[fd] && this.fd[fd].open) {
    console.warn(`readFile: file ${fname} hasn't been closed yet!`);
  }

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
  const filePath = assertLeadingSlash(normalizePath(fname));
  const maybeFd = Object.values(this.fd).find(({ path }) => path === filePath);

  if (maybeFd) {
    delete this.fd[maybeFd];
  } else {
    console.error(`While trying to unlink ${filePath}, path not found`);
  }
};

WASI.prototype.mkdir = function (dirname /* string */) {
  const cleanPath = assertLeadingSlash(normalizePath(dirname));
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
    };
  }
};
