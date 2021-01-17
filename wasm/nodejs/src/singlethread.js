import libcsoundFactory from "@csound/browser/dist/factory.mjs";
import loadWasm from "./module.js";

// const speaker = new Speaker({
//   channels: 2, // 2 channels
//   bitDepth: 16, // 16-bit samples
//   sampleRate: 44100, // 44,100 Hz sample rate
// });
// let endResolver;
// const endPromise = new Promise((resolve) => {
//   endResolver = resolve;
// });

// const stream = new Readable();

// stream._read = function (chunk) {
//   console.log(chunk.toString());
// };
// stream.on("end", (x) => console.log("end", x) || endResolver());
// stream.pipe(speaker);
// await endPromise;

export default class SingleThread {
  constructor() {
    this.initialize = this.initialize.bind(this);
  }

  async initialize({ withPlugins = [] }) {
    console.log("init", 111);
    const wasm = await loadWasm({ withPlugins });
    console.log("init", 222);
    const csoundApi = libcsoundFactory(wasm);
    console.log("init", 333);
    console.log(csoundApi);
    this.csoundApi = csoundApi;
    return csoundApi;
  }
}
