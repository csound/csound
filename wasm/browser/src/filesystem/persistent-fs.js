import { createFsFromVolume } from "memfs";
import { Volume } from "memfs/lib/volume";
import { difference } from "ramda";

// because workers come and go, for users to have some
// persistency, we'll sync a non-worker storage
// with the filesystems spawned on threads  * @extends {typeof import("memfs").IFs}
export const persistentStorage = new Volume();

/**
 * The in-browser filesystem based on nodejs's
 * built-in module "fs"
 * @name fs
 * @memberof CsoundObj
 * @type {IFs:memfs}
 */
export const persistentFilesystem = createFsFromVolume(persistentStorage);

// modified from @wasmer/wasmfs
function fromJSONFixed(vol, json) {
  const seperator = "/";
  for (let filename in json) {
    filename = filename.replace(/^\/sandbox\//i, "");
    const data = json[filename];
    const isDirectory = data ? Object.getPrototypeOf(data) === null : data === null;
    if (!isDirectory) {
      const steps = filename.split(seperator);
      if (steps.length > 1) {
        const dirname = seperator + steps.slice(0, -1).join(seperator);
        vol.mkdirpBase(dirname, 0o777);
      }
      vol.writeFileSync(filename, data || "");
    } else {
      vol.mkdirpBase(filename, 0o777);
    }
  }
}

// modified from @wasmer/wasmfs
function _toJSON(link, json = {}) {
  let isEmpty = true;

  for (const name in link.children) {
    isEmpty = false;

    const child = link.getChild(name);
    if (child) {
      const node = child.getNode();
      if (node && node.isFile()) {
        const filename = child.getPath();
        json[filename] = node.getBuffer();
      } else if (node && node.isDirectory()) {
        _toJSON(child, json);
      }
    }
  }

  const directoryPath = link.getPath();

  if (directoryPath && isEmpty) {
    delete json[directoryPath];
  }

  return json;
}

// modified from @wasmer/wasmfs
function toJSONFixed(volume, paths, json = {}, isRelative = false) {
  const links = [];

  if (paths) {
    if (!Array.isArray(paths)) paths = [paths];
    for (const path of paths) {
      const filename = path.split("/").slice(-1)[0];
      const link = volume.getResolvedLink(filename);
      if (!link) continue;
      links.push(link);
    }
  } else {
    links.push(volume.root);
  }

  if (links.length === 0) return json;
  for (const link of links) _toJSON(link, json, isRelative ? link.getPath() : "");
  return json;
}

const lastmods = {};

export const syncPersistentStorage = (workerStorage) =>
  fromJSONFixed(persistentStorage, workerStorage);

export const getModifiedPersistentStorage = () => {
  const currentFs = toJSONFixed(persistentStorage, "/");
  const needsSync = {};

  for (const dataKey of Object.keys(currentFs)) {
    const lastModified = persistentFilesystem.statSync(dataKey).mtimeMs;
    if (!lastmods[dataKey]) {
      needsSync[dataKey] = currentFs[dataKey];
      lastmods[dataKey] = lastModified;
    } else if (lastmods[dataKey] !== lastModified) {
      needsSync[dataKey] = currentFs[dataKey];
      lastmods[dataKey] = lastModified;
    }
  }

  const needsUnlink = difference(Object.keys(lastmods), Object.keys(currentFs));

  if (needsUnlink.length > 0) {
    needsSync.__unlink = needsUnlink;
  }

  return needsSync;
};
