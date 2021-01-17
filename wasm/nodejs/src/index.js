import SingleThread from "./singlethread.js";

export async function Csound({ withPlugins = [], useWorker = false } = {}) {
  console.log(111);
  const instance = new SingleThread({});
  console.log(222);
  const csoundApi = await instance.initialize({ withPlugins });
  console.log(333);
  return csoundApi;
}

export default Csound;
