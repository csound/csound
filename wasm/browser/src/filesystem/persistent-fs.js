goog.provide("csound.filesystem.PersistentFs");
goog.require("goog.async.Deferred");
goog.require("goog.db.IndexedDb");
goog.require("goog.db.Transaction");
goog.require("goog.string");
goog.require("goog.string.path");

const TransactionMode = goog.db.Transaction.TransactionMode;
const IndexedDb = goog.db.IndexedDb;

csound.filesystem.PersistentFs = function () {
  this.initDb = this.initDb.bind(this);
  this.ls = this.ls.bind(this);
  this.write = this.write.bind(this);
};

csound.filesystem.PersistentFs.prototype.initDb = function () {};

function cutRootSlash(s = "") {
  return s.replace(/^\//g, "");
}

csound.filesystem.PersistentFs.prototype.write = function (filename, data) {
  const defered = new goog.async.Deferred();

  if (typeof filename !== "string") {
    throw new Error("fs.writeFile - filename must be of type string");
  }

  if (!(data instanceof Uint8Array)) {
    throw new Error("fs.writeFile - data must be of type Uint8Array");
  }

  filename = cutRootSlash(goog.string.path.normalizePath(filename));
  const nextVersionDefer = new goog.async.Deferred();

  goog.db.openDatabase("csound").addCallback((lastDb) => {
    const currentVersion = lastDb.getVersion();
    const nextVersion = currentVersion + 1;
    lastDb.close();
    nextVersionDefer.callback(nextVersion);
  });

  nextVersionDefer.then((nextVersion) => {
    const onBlocked = () =>
      defered.errback(
        `fs.write - couldn't write ${filename} to fs.` +
          "\n" +
          `Maybe csound is currently running on a different thread/tab?`
      );

    const onUpgradeNeeded = (ev, nextDb, tx) => {
      if (!nextDb.getObjectStoreNames().contains(filename)) {
        nextDb.createObjectStore(filename);
      }
    };

    goog.db.openDatabase("csound", nextVersion, onUpgradeNeeded, onBlocked).then((nextDb) => {
      const putTx = nextDb.createTransaction(
        [filename],
        goog.db.Transaction.TransactionMode.READ_WRITE
      );

      const store = putTx.objectStore(filename);

      if (nextDb.getObjectStoreNames().contains(filename)) {
        store.clear();
      }
      store.put(data, 1);
      nextDb.close();
      defered.awaitDeferred(putTx.wait());
      defered.callback();
    });
  });

  return defered;
};

csound.filesystem.PersistentFs.prototype.ls = function (dirName) {
  const defered = new goog.async.Deferred();
  let dirFilter = "";
  if (!dirName || dirName === "/" || typeof dirName !== "string") {
    dirFilter = "";
  } else {
    dirName = goog.string.path.normalizePath(dirName);
    dirName = curRootSlash(dirName);
    dirFilter = dirName;
  }
  goog.db.openDatabase("csound").addCallback((db) => {
    const allItems = Array.from(db.getObjectStoreNames());

    const result = allItems.reduce((a, i) => {
      if (dirFilter === "") {
        const cand = i.replace(/\/.*/, "");
        if (!a.includes(cand)) {
          a.push(cand);
        }
      } else {
        if (i.startsWith(dirFilter) && !a.includes(cutRootSlash(i.replace(dirFilter)))) {
          a.push(cutRootSlash(i.replace(dirFilter)));
        }
      }
      return a;
    }, []);
    db.close();
    defered.callback(result);
  });
  return defered;
};

// for singlethread-wasi which has no direct indexedDb access
csound.filesystem.PersistentFs.prototype.transferDbBeforeStart = function (dirName) {
  const defered = new goog.async.Deferred();

  goog.db.openDatabase("csound").addCallback(async (db) => {
    const dbAssets = db.getObjectStoreNames();
    const tx = await db.createTransaction(dbAssets, goog.db.Transaction.TransactionMode.READ_ONLY);
    const dbFs = {};
    const buffers = [];

    for (const dbAsset of dbAssets) {
      const handle = await tx.objectStore(dbAsset).getAll();
      dbFs[dbAsset] = handle;
      for (const arr of handle) {
        buffers.push(arr.buffer);
      }
    }
    defered.succeed({ data: dbFs, buffers });
  });
  // .addErrback((error) => defered.cancel(error));

  return defered;
};

// const curDb = await goog.db.openDatabase("csound");
// const tx = await curDb.createTransaction(["/csound/xxx.wav"], TransactionMode.READ_ONLY);
// const objectStore = tx.objectStore("/csound/xxx.wav");
// const allData = await objectStore.getAll();
// const newFile = new File(allData, "xxx.wav", { type: "audio/wav" });
// console.log(
//   "curDb",
//   curDb,
//   tx,
//   objectStore,
//   allData,
//   newFile,
//   URL.createObjectURL(newFile)
// );
// break;
