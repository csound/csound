import SingleThread from "./singlethread.js";

export async function Csound({ withPlugins = [], useWorker = false } = {}) {
  console.log(111);
  const instance = new SingleThread({});
  console.log(222);
  return await instance.initialize({ withPlugins });
}

export default Csound;
