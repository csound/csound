export const dlinit = (hostInstance, pluginInstance, table, csoundInstance) => {
  if (pluginInstance.exports.csoundModuleInit) {
    const csoundModuleCreate = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundModuleInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundModuleDestroy = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundModuleErrorCodeToString = new WebAssembly.Global(
      { value: "i32", mutable: true },
      0,
    );

    let tableEnd = table.length;

    if (typeof pluginInstance.exports.csoundModuleCreate === "function") {
      table.grow(1);
      csoundModuleCreate.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports.csoundModuleCreate);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports.csoundModuleInit === "function") {
      table.grow(1);
      csoundModuleInit.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports.csoundModuleInit);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports.csoundModuleDestroy === "function") {
      table.grow(1);
      csoundModuleDestroy.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports.csoundModuleDestroy);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports.csoundModuleErrorCodeToString === "function") {
      table.grow(1);
      csoundModuleErrorCodeToString.value = tableEnd;
      table.set(tableEnd, pluginInstance.exports.csoundModuleErrorCodeToString);
      tableEnd += 1;
    }

    hostInstance.exports.csoundWasiLoadPlugin(
      csoundInstance,
      csoundModuleCreate,
      csoundModuleInit,
      csoundModuleDestroy,
      csoundModuleErrorCodeToString,
    );
  } else if (pluginInstance.exports.csound_opcode_init || pluginInstance.exports.csound_fgen_init) {
    const csoundOpcodeInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);
    const csoundFgenInit = new WebAssembly.Global({ value: "i32", mutable: true }, 0);

    let tableEnd = table.length;

    if (typeof pluginInstance.exports.csound_opcode_init === "function") {
      csoundOpcodeInit.value = tableEnd;
      table.grow(1);
      table.set(tableEnd, pluginInstance.exports.csound_opcode_init);
      tableEnd += 1;
    }

    if (typeof pluginInstance.exports.csound_fgen_init === "function") {
      csoundFgenInit.value = tableEnd;
      table.grow(1);
      table.set(tableEnd, pluginInstance.exports.csound_fgen_init);
      tableEnd += 1;
    }

    hostInstance.exports.csoundWasiLoadOpcodeLibrary(
      csoundInstance,
      csoundFgenInit,
      csoundOpcodeInit,
    );
  } else {
    console.error("Plugin doesn't export nececcary functions to quality as csound plugin.");
  }
};
