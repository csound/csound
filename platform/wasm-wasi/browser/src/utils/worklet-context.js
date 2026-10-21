/* Copyright (c) The Csound Developers. Licensed under Apache-2.0. */

// Main and compatibility bundles share an AudioContext but have separate module
// scopes. Keep registration and node IDs together so neither bundle registers
// the same processor twice or reuses an ID during concurrent starts.
const registryKey = Symbol.for("@csound/browser.worklet-contexts.v1");
if (!globalThis[registryKey])
  Object.defineProperty(globalThis, registryKey, { value: new WeakMap() });
const contexts = globalThis[registryKey];

const contextState = (audioContext) => {
  let state = contexts.get(audioContext);
  if (!state) {
    // Stable field names let separately optimized bundles share this state.
    state = {};
    state["modules"] = new Map();
    state["nextNodeId"] = 0;
    contexts.set(audioContext, state);
  }
  return state;
};

export const registerWorkletModule = (audioContext, processorName, register) => {
  const modules = contextState(audioContext)["modules"];
  const previous = modules.get(processorName);
  if (previous) return previous;
  const pending = Promise.resolve()
    .then(register)
    .catch((error) => {
      modules.delete(processorName);
      throw error;
    });
  modules.set(processorName, pending);
  return pending;
};

export const nextWorkletNodeId = (audioContext) => {
  const state = contextState(audioContext);
  return `audioWorklet${state["nextNodeId"]++}`;
};
