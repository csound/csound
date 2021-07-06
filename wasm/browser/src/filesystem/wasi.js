goog.provide("csound.filesystem.WASI");

goog.require("csound.filesystem.constants");
goog.require("csound.utils.text_encoders");
goog.require("goog.async.Deferred");
goog.require("goog.fs");
goog.require("goog.fs.DirectoryEntry.Behavior");
goog.require("goog.string");
goog.require("goog.string.path");
goog.require("goog.db");
goog.require("goog.db.IndexedDb");
goog.require("goog.db.Transaction");

/** @define {boolean} */
const DEBUG_WASI = goog.define("DEBUG_WASI", false);

function shouldOpenReader(rights) {
  return (
    (rights &
      (csound.filesystem.constants.WASI_RIGHT_FD_READ |
        csound.filesystem.constants.WASI_RIGHT_FD_READDIR)) !==
    goog.global.BigInt(0)
  );
}

function performanceNowPoly() {
  if (typeof performance === "undefined" || typeof performance.now === "undefined") {
    var nowOffset = Date.now();
    return Date.now() - nowOffset;
  } else {
    return performance.now();
  }
}

csound.filesystem.WASI = function ({ preopens }) {
  this.fd = {
    0: { fd: 0, path: "/dev/stdin", seekPos: goog.global.BigInt(0) },
    1: { fd: 1, path: "/dev/stdout", seekPos: goog.global.BigInt(0) },
    2: { fd: 2, path: "/dev/stderr", seekPos: goog.global.BigInt(0) },
    // currently this cwd is now a totally hidden, at least while sandboxing
    // isn't forced in browsers
    3: { fd: 3, path: "/csound", type: "dir", seekPos: goog.global.BigInt(0) },
  };
  this.dbFs = {};
  this.nextFd = 4;
  this.getMemory = this.getMemory.bind(this);
  this.getHandle = this.getHandle.bind(this);
  this.CPUTIME_START = 0;
};

csound.filesystem.WASI.prototype.initDb = async function () {
  if (typeof AudioWorkletGlobalScope === "undefined") {
    const lastDb = this.db || (await goog.db.openDatabase("csound"));
    // +2 because we open and close once in between
    this.nextDbVersion = lastDb ? lastDb.getVersion() + 2 : 1;
    lastDb && lastDb.close();
    delete this.db;
  }
};

csound.filesystem.WASI.prototype.syncDbBeforeStart = async function () {
  // TODO not to include entire db on performance (cwd scopes)
  if (typeof AudioWorkletGlobalScope === "undefined") {
    this.db = this.db || (await goog.db.openDatabase("csound"));
    const dbAssets = this.db.getObjectStoreNames();
    this.tx = await this.db.createTransaction(
      dbAssets,
      goog.db.Transaction.TransactionMode.READ_ONLY
    );

    for (const dbAsset of dbAssets) {
      const handle = await this.tx.objectStore(dbAsset).getAll();
      this.dbFs[dbAsset] = handle;
    }
  }
};

csound.filesystem.WASI.prototype.syncDbAfterEnd = function () {
  const defered = new goog.async.Deferred();

  if (this.db) {
    if (this.tx) {
      this.tx.removeAllListeners();
      this.tx.dispose();
    }

    this.db.close();
    delete this.db;
    delete this.tx;
  }

  const onUpgradeNeeded = (ev, db, tx) => {
    const currentStores = db.getObjectStoreNames();
    const userFds = Object.keys(this.fd).filter((k) => k > 3);

    for (const fd of userFds) {
      const fdPath = this.fd[fd].path;
      if (!currentStores.contains(fdPath)) {
        db.createObjectStore(fdPath);
      }
    }
  };

  const onBlocked = (evt) =>
    console.error(`Upgrade to version ${this.nextDbVersion} is blocked.`, evt, this.db);

  goog.db.openDatabase("csound", this.nextDbVersion, onUpgradeNeeded, onBlocked).then((db) => {
    const userFds = Object.keys(this.fd).filter((k) => k > 3);
    const currentStores = db.getObjectStoreNames();
    const deferedTx = new goog.async.Deferred();

    userFds.forEach((fd) => {
      const fdPath = this.fd[fd].path;
      if (currentStores.contains(fdPath)) {
        const putTx = db.createTransaction(
          [fdPath],
          goog.db.Transaction.TransactionMode.READ_WRITE
        );
        const store = putTx.objectStore(fdPath);

        store.clear();

        if (this.fd[fd].buffers) {
          this.fd[fd].buffers.forEach((buffer, index) => store.put(buffer, index + 1));
        }
        deferedTx.awaitDeferred(putTx.wait());
      }
    });
    deferedTx.addFinally(() => defered.callback());
    deferedTx.callback({});
    db.close();
  });

  return defered;
};

// csound.filesystem.WASI.prototype.createFs = async function (size = 2e9) {
//   // 2gb
//   this.fsHandle = await goog.fs.getTemporary(size);
//   this.fs = this.fsHandle.getBrowserFileSystem();
//   this.fsRoot = await this.fsHandle
//     .getRoot()
//     .getDirectory("csound", goog.fs.DirectoryEntry.Behavior.CREATE);
//   // the root sandbox is the /csound dir handle
//   // this.fd[3].handle = this.fsRoot;
// };

/**
 * @function
 * @param {!LibcsoundUncloned} csound
 * @return {Boolean}
 */
// csound.filesystem.WASI.prototype.sensevents = async function (csound) {
//   if (this.senseventsNative) {
//     const done = this.senseventsNative(csound);
//     console.log("done", done);
//     if (done !== 0) {
//       return false;
//     } else {
//       return done;
//     }
//   } else {
//     return false;
//   }
// };

/**
 * @function
 * @param {!WebAssembly.Instance} instance
 */
csound.filesystem.WASI.prototype.start = function (instance) {
  this.CPUTIME_START = performanceNowPoly();
  const exports = instance.exports;
  exports._start();
};

/**
 * @function
 * @param {!WebAssembly.Module} instance
 */
csound.filesystem.WASI.prototype.getImports = function (module) {
  const options = {};
  for (const neededImport of WebAssembly.Module.imports(module)) {
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
csound.filesystem.WASI.prototype.setMemory = function (memory) {
  this.memory = memory;
};

/**
 * @function
 * @return {DataView}
 */
csound.filesystem.WASI.prototype.getMemory = function () {
  if (!this.view || this.view.buffer.byteLength === 0) {
    this.view = new DataView(this.memory.buffer);
  }
  return this.view;
};

csound.filesystem.WASI.prototype.msToNs = function (ms) {
  const msInt = Math.trunc(ms);
  const decimal = goog.global.BigInt(Math.round((ms - msInt) * 1000000));
  const ns = goog.global.BigInt(msInt) * goog.global.BigInt(1000000);
  return ns + decimal;
};

csound.filesystem.WASI.prototype.now = function (clockId) {
  switch (clockId) {
    case csound.filesystem.constants.WASI_CLOCK_MONOTONIC:
      // case csound.filesystem.constants.WASI_CLOCK_REALTIME:
      return Math.floor(performanceNowPoly());
    case csound.filesystem.constants.WASI_CLOCK_REALTIME:
      return this.msToNs(Date.now());
    case csound.filesystem.constants.WASI_CLOCK_PROCESS_CPUTIME_ID:
    case csound.filesystem.constants.WASI_CLOCK_THREAD_CPUTIME_ID:
      // return bindings.hrtime(CPUTIME_START)
      return Math.floor(performanceNowPoly() - this.CPUTIME_START);
    default:
      return 0;
  }
};

csound.filesystem.WASI.prototype.args_get = function (argv, argvBuf) {
  if (DEBUG_WASI) {
    console.log("args_get", argv, argvBuf);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.args_sizes_get = function (argc, argvBufSize) {
  if (DEBUG_WASI) {
    console.log("args_sizes_get", argc, argvBufSize, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.clock_res_get = function (clockId, resolution) {
  if (DEBUG_WASI) {
    console.log("args_get", clockId, resolution, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.clock_time_get = function (clockId, precision, time) {
  if (DEBUG_WASI) {
    console.log("clock_time_get", clockId, precision, time, arguments);
  }
  const memory = this.getMemory();
  const nextTime = this.now(clockId);
  memory.setBigUint64(time, goog.global.BigInt(nextTime), true);
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.environ_get = function (environ, environBuf) {
  if (DEBUG_WASI) {
    console.log("environ_get", environ, environBuf, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.environ_sizes_get = function (environCount, environBufSize) {
  if (DEBUG_WASI) {
    console.log("environ_sizes_get", environCount, environBufSize, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_advise = function (fd, offset, len, advice) {
  if (DEBUG_WASI) {
    console.log("fd_advise", fd, offset, len, advice, arguments);
  }
  return csound.filesystem.constants.WASI_ENOSYS;
};
csound.filesystem.WASI.prototype.fd_allocate = function (fd, offset, len) {
  if (DEBUG_WASI) {
    console.log("fd_allocate", fd, offset, len, arguments);
  }
  return csound.filesystem.constants.WASI_ENOSYS;
};
csound.filesystem.WASI.prototype.fd_close = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_close", fd, arguments);
  }

  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_datasync = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_datasync", fd, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};

// always write access in browser scope
csound.filesystem.WASI.prototype.fd_fdstat_get = function (fd, bufPtr) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_get", fd, bufPtr, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_fdstat_set_flags = function (fd, flags) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_set_flags", fd, flags, arguments);
  }
  return csound.filesystem.constants.WASI_ENOSYS;
};
csound.filesystem.WASI.prototype.fd_fdstat_set_rights = function (
  fd,
  fsRightsBase,
  fsRightsInheriting
) {
  if (DEBUG_WASI) {
    console.log("fd_fdstat_set_rights", fd, fsRightsBase, fsRightsInheriting, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.fd_filestat_get = function (fd, bufPtr) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_get", fd, bufPtr, arguments);
  }
  let filesize = 0;
  const handle = this.getHandle(fd);
  if (handle) {
    filesize = handle.reduce(function (acc, uintArray) {
      return acc + uintArray.byteLength;
    }, 0);
  }

  const memory = this.getMemory();
  memory.setBigUint64(bufPtr, goog.global.BigInt(fd), true);
  bufPtr += 8;
  memory.setBigUint64(bufPtr, goog.global.BigInt(fd), true);
  bufPtr += 8;
  memory.setUint8(bufPtr, csound.filesystem.constants.WASI_FILETYPE_REGULAR_FILE);
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

  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.fd_filestat_set_size = function (fd, newSize) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_set_size", fd, newSize, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_filestat_set_times = function (
  fd,
  stAtim,
  stMtim,
  filestatFags
) {
  if (DEBUG_WASI) {
    console.log("fd_filestat_set_times", fd, stAtim, stMtim, filestatFags, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_pread = function (fd, iovs, iovsLen, offset, nread) {
  if (DEBUG_WASI) {
    console.log("fd_pread", fd, iovs, iovsLen, offset, nread, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_prestat_dir_name = function (fd, pathPtr, pathLen) {
  if (!this.fd[fd] && !this.fd[fd - 1]) {
    return csound.filesystem.constants.WASI_EBADF;
  }

  const { path: dirName } = this.fd[fd];

  const memory = this.getMemory();
  const dirNameBuf = csound.utils.text_encoders.encoder.encode(dirName);
  new Uint8Array(this.memory.buffer).set(dirNameBuf, pathPtr);

  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_prestat_get = function (fd, bufPtr) {
  if (!this.fd[fd]) {
    return csound.filesystem.constants.WASI_EBADF;
  }
  const { path: dirName } = this.fd[fd];
  const memory = this.getMemory();

  const dirNameBuf = csound.utils.text_encoders.encoder.encode(dirName);
  memory.setUint8(bufPtr, csound.filesystem.constants.WASI_PREOPENTYPE_DIR);
  memory.setUint32(bufPtr + 4, dirNameBuf.byteLength, true);
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_pwrite = function (fd, iovs, iovsLen, offset, nwritten) {
  console.log("fd_pwrite", fd, iovs, iovsLen, offset, nwritten, arguments);
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_read = function (fd, iovs, iovsLen, nread) {
  if (DEBUG_WASI) {
    console.log("fd_read", fd, iovs, iovsLen, nread, arguments);
  }
  const handle = this.getHandle(fd);

  if (!handle) {
    console.error("Reading non existent file handle", fd, this.fd[fd]);
    return;
  }

  const memory = this.getMemory();

  let read = Number(this.fd[fd].seekPos);
  let thisRead = 0;

  for (let i = 0; i < iovsLen; i++) {
    const ptr = iovs + i * 8;
    const buf = memory.getUint32(ptr, true);
    const bufLen = memory.getUint32(ptr + 4, true);
    thisRead += bufLen;

    Array.from({ length: bufLen }, (_, i) => i).reduce(
      (acc, currRead) => {
        const [chunkIdx, chunkOffset] = acc;
        let currChunkIndex = 0;
        let currChunkOffset = 0;

        if (currRead === 0) {
          let found = false;
          let leadup = 0;
          while (!found) {
            if (leadup <= read && handle[currChunkIndex].byteLength + leadup > read) {
              found = true;
              currChunkOffset = read - leadup;
            } else {
              leadup += handle[currChunkIndex].byteLength;
              currChunkIndex += 1;
            }
          }
        } else {
          currChunkIndex = chunkIdx;
          currChunkOffset = chunkOffset;
        }

        memory.setUint8(buf + currRead, handle[currChunkIndex][currChunkOffset]);

        if (currChunkOffset + 1 >= handle[currChunkIndex].byteLength) {
          currChunkIndex = chunkIdx + 1;
          currChunkOffset = 0;
        } else {
          currChunkOffset += 1;
        }

        return [currChunkIndex, currChunkOffset];
      },
      [0, 0]
    );
    read += bufLen;
  }

  this.fd[fd].seekPos = goog.global.BigInt(read);
  memory.setUint32(nread, thisRead, true);
  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.fd_readdir = function (fd, bufPtr, bufLen, cookie, bufusedPtr) {
  if (DEBUG_WASI) {
    console.log("fd_readdir", fd, bufPtr, bufLen, cookie, bufusedPtr, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_renumber = function (from, to) {
  if (DEBUG_WASI) {
    console.log("fd_renumber", from, to, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_seek = function (fd, offset, whence, newOffsetPtr) {
  if (DEBUG_WASI) {
    console.log("fd_seek", fd, offset, whence, newOffsetPtr, arguments);
  }
  const memory = this.getMemory();

  switch (whence) {
    case csound.filesystem.constants.WASI_WHENCE_CUR:
      this.fd[fd].seekPos =
        (this.fd[fd].seekPos ? this.fd[fd].seekPos : goog.global.BigInt(0)) +
        goog.global.BigInt(offset);
      break;
    case csound.filesystem.constants.WASI_WHENCE_END:
      const currLength = this.fd[fd].writer
        ? goog.global.BigInt(this.fd[fd].writer.length)
        : goog.global.BigInt(0);
      this.fd[fd].seekPos = currentLength + BigInt(offset);
      break;
    case csound.filesystem.constants.WASI_WHENCE_SET:
      this.fd[fd].seekPos = BigInt(offset);
      break;
  }

  memory.setBigUint64(newOffsetPtr, this.fd[fd].seekPos, true);

  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_sync = function (fd) {
  if (DEBUG_WASI) {
    console.log("fd_sync", fd, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.fd_tell = function (fd, offsetPtr) {
  if (DEBUG_WASI) {
    console.log("fd_tell", fd, offsetPtr, arguments);
  }
  const memory = this.getMemory();

  if (!this.fd[fd].seekPos) {
    this.fd[fd].seekPos = goog.global.BigInt(0);
  }

  memory.setBigUint64(offsetPtr, this.fd[fd].seekPos, true);

  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.fd_write = function (fd, iovs, iovsLen, nwritten) {
  const memory = this.getMemory();
  this.fd[fd].buffers = this.fd[fd].buffers || [];

  // append-only, if starting new write from beginning
  // we are then assuming an overwrite (until this bites us)
  if (this.fd[fd].seekPos === goog.global.BigInt(0) && this.fd[fd].buffers.length > 0) {
    this.fd[fd].buffers = [];
  }
  let written = 0;

  for (let i = 0; i < iovsLen; i++) {
    const ptr = iovs + i * 8;
    const buf = memory.getUint32(ptr, true);
    const bufLen = memory.getUint32(ptr + 4, true);
    written += bufLen;
    const chunk = new Uint8Array(memory.buffer, buf, bufLen);
    this.fd[fd].buffers.push(chunk.slice(0, bufLen));
  }

  this.fd[fd].seekPos += goog.global.BigInt(written);
  memory.setUint32(nwritten, written, true);
  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.path_create_directory = function (fd, pathPtr, pathLen) {
  if (DEBUG_WASI) {
    console.log("path_create_directory", fd, pathPtr, pathLen, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.path_filestat_get = function (
  fd,
  flags,
  pathPtr,
  pathLen,
  bufPtr
) {
  if (DEBUG_WASI) {
    console.log("path_filestat_get", fd, flags, pathPtr, pathLen, bufPtr, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.path_filestat_set_times = function (
  fd,
  dirflags,
  pathPtr,
  pathLen,
  stAtim,
  stMtim,
  fstflags
) {
  if (DEBUG_WASI) {
    console.log(
      "path_filestat_set_times",
      fd,
      dirflags,
      pathPtr,
      pathLen,
      stAtim,
      stMtim,
      fstflags,
      arguments
    );
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.path_link = function (
  oldFd,
  oldFlags,
  oldPath,
  oldPathLen,
  newFd,
  newPath,
  newPathLen
) {
  if (DEBUG_WASI) {
    console.log(
      "path_link",
      oldFd,
      oldFlags,
      oldPath,
      oldPathLen,
      newFd,
      newPath,
      newPathLen,
      arguments
    );
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.path_open = function (
  dirfd,
  dirflags,
  pathPtr,
  pathLen,
  oflags,
  fsRightsBase,
  fsRightsInheriting,
  fsFlags,
  fd
) {
  if (DEBUG_WASI) {
    console.log(
      "path_open",
      dirfd,
      dirflags,
      pathPtr,
      pathLen,
      oflags,
      fsRightsBase,
      fsRightsInheriting,
      fsFlags,
      fd,
      arguments
    );
  }
  const memory = this.getMemory();
  const dirPath = (this.fd[dirfd] || { path: "/" })["path"];
  const pathOpenBytes = new Uint8Array(this.memory.buffer, pathPtr, pathLen);
  const pathOpenStr = decoder.decode(pathOpenBytes);
  const pathOpen = goog.string.path.normalizePath(
    goog.string.path.join(dirfd === 3 ? "" : dirPath, pathOpenStr)
  );

  if (DEBUG_WASI) {
    console.log(";; opening path", pathOpen, "withREader", shouldOpenReader(fsRightsBase));
  }

  if (pathOpen.startsWith("..") || pathOpen === "._" || pathOpen === ".AppleDouble") {
    return csound.filesystem.constants.WASI_EBADF;
  }

  const alreadyExists = Object.values(this.fd).find((entry) => entry.path === pathOpen);
  let actualFd;

  if (alreadyExists) {
    actualFd = alreadyExists.fd;
  } else {
    actualFd = this.nextFd;
    this.nextFd += 1;
  }

  let fileType = "file";

  this.fd[actualFd] = {
    fd: actualFd,
    path: pathOpen,
    type: fileType,
    seekPos: goog.global.BigInt(0),
  };

  if ((oflags & csound.filesystem.constants.WASI_O_DIRECTORY) !== 0) {
    fileType = "dir";
  } else {
    this.fd[actualFd].buffers = [];
  }

  if (shouldOpenReader(fsRightsBase)) {
    if (DEBUG_WASI) {
      console.log("should open a read handle for", pathOpen);
    }
  }

  memory.setUint32(fd, actualFd, true);

  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.path_readlink = function (
  fd,
  pathPtr,
  pathLen,
  buf,
  bufLen,
  bufused
) {
  if (DEBUG_WASI) {
    console.log("path_readlink", fd, pathPtr, pathLen, buf, bufLen, bufused, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.path_remove_directory = function (fd, pathPtr, pathLen) {
  if (DEBUG_WASI) {
    console.log("path_remove_directory", fd, pathPtr, pathLen);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.path_rename = function (
  oldFd,
  oldPath,
  oldPathLen,
  newFd,
  newPath,
  newPathLen
) {
  if (DEBUG_WASI) {
    console.log("path_rename", oldFd, oldPath, oldPathLen, newFd, newPath, newPathLen, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.path_symlink = function (
  oldPath,
  oldPathLen,
  fd,
  newPath,
  newPathLen
) {
  if (DEBUG_WASI) {
    console.log("path_symlink", oldPath, oldPathLen, fd, newPath, newPathLen, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.path_unlink_file = function (fd, pathPtr, pathLen) {
  if (fd > 3) {
    if (DEBUG_WASI) {
      console.log("path_unlink_file", fd, pathPtr, pathLen, arguments);
    }
    // actual file removal goes here
  }

  return csound.filesystem.constants.WASI_ESUCCESS;
};

csound.filesystem.WASI.prototype.poll_oneoff = function (sin, sout, nsubscriptions, nevents) {
  if (DEBUG_WASI) {
    console.log("poll_oneoff", sin, sout, nsubscriptions, nevents, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.proc_exit = function (rval) {
  if (DEBUG_WASI) {
    console.log("proc_exit", rval, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.proc_raise = function (sig) {
  if (DEBUG_WASI) {
    console.log("proc_raise", sig, arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.random_get = function (bufPtr, bufLen) {
  if (DEBUG_WASI) {
    console.log("random_get", bufPtr, bufLen);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.sched_yield = function () {
  if (DEBUG_WASI) {
    console.log("sched_yield", arguments);
  }
  return csound.filesystem.constants.WASI_ESUCCESS;
};
csound.filesystem.WASI.prototype.sock_recv = function () {
  if (DEBUG_WASI) {
    console.log("sock_recv", arguments);
  }
  return csound.filesystem.constants.WASI_ENOSYS;
};
csound.filesystem.WASI.prototype.sock_send = function () {
  if (DEBUG_WASI) {
    console.log("sock_send", arguments);
  }
  return csound.filesystem.constants.WASI_ENOSYS;
};
csound.filesystem.WASI.prototype.sock_shutdown = function () {
  if (DEBUG_WASI) {
    console.log("sock_shutdown", arguments);
  }
  return csound.filesystem.constants.WASI_ENOSYS;
};

// helpers

csound.filesystem.WASI.prototype.getHandle = function (fd) {
  if (this.fd[fd]) {
    const assetKey = this.fd[fd].path;
    const handle = assetKey && this.dbFs[assetKey];

    if (handle && Array.isArray(handle) && handle.length > 0 && handle[0] instanceof Uint8Array) {
      return handle;
    }
  }
};
