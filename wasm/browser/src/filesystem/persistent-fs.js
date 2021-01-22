import { createFsFromVolume } from "memfs";
import { filenameToSteps, pathToFilename, Volume } from "memfs/lib/volume";
import path from "path";

// because workers come and go, for users to have some
// persistency, we'll sync a non-worker storage
// with the filesystems spawned on threads
export const persistentStorage = new Volume();
export const persistentFilesystem = createFsFromVolume(persistentStorage);

// modified from @wasmer/wasmfs
function fromJSONFixed(vol, json) {
  const seperator = "/";
  for (let filename in json) {
    filename = filename.replace(/^\/sandbox\//i, "");
    const data = json[filename];
    const isDirectory = data ? Object.getPrototypeOf(data) === null : data === null;
    if (!isDirectory) {
      const steps = filenameToSteps(filename);
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
        let filename = child.getPath();
        if (path) filename = path.relative(path, filename);
        json[filename] = node.getBuffer();
      } else if (node && node.isDirectory()) {
        _toJSON(child, json, path);
      }
    }
  }

  let directoryPath = link.getPath();

  if (path) directoryPath = path.relative(path, directoryPath);

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
      const filename = pathToFilename(path);
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

export const syncPersistentStorage = (workerStorage) => {
  fromJSONFixed(persistentStorage, workerStorage);
};

export const getPersistentStorage = () => toJSONFixed(persistentStorage, "/");
