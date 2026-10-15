// A capability check, not an audio benchmark or a pthread runtime.
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { Worker, isMainThread, parentPort, workerData } from 'node:worker_threads';

const MASK = 0x5a5a5a5a;
const COUNT = 100000;
const TIMEOUT_MS = 30000;
const unpack = value => [Number(value >> 32n), Number(value & 0xffffffffn)];

function waitForMessage(worker, phase) {
  return new Promise((resolve, reject) => {
    const cleanup = () => {
      clearTimeout(timer);
      worker.off('message', onMessage);
      worker.off('error', onError);
      worker.off('exit', onExit);
    };
    const onMessage = message => { cleanup(); resolve(message); };
    const onError = error => { cleanup(); reject(error); };
    const onExit = code => onError(new Error(`${phase}: worker exited with code ${code}`));
    const timer = setTimeout(() => onError(new Error(`${phase} timed out`)), TIMEOUT_MS);
    worker.once('message', onMessage);
    worker.once('error', onError);
    worker.once('exit', onExit);
  });
}

if (isMainThread) {
  const bytes = await readFile(process.argv[2]);
  const module = await WebAssembly.compile(bytes);
  const memory = new WebAssembly.Memory({ initial: 4, maximum: 4, shared: true });
  const main = await WebAssembly.instantiate(module, { env: { memory } });
  const worker = new Worker(new URL(import.meta.url), { workerData: { module, memory } });
  let cancelled = false;
  try {
    assert.equal(await waitForMessage(worker, 'Startup'), 'ready');
    // Both instances exist before reset; this does not test C runtime init/TLS.
    main.exports.queue_reset();
    assert.equal(main.exports.queue_pop(), -1n);
    for (let n = 0; n < 64; n++) assert.equal(main.exports.queue_push(n, n ^ MASK), 1);
    assert.equal(main.exports.queue_push(64, 64 ^ MASK), 0);
    for (let n = 0; n < 64; n++) assert.deepEqual(unpack(main.exports.queue_pop()), [n, n ^ MASK]);
    assert.equal(main.exports.queue_pop(), -1n);
    main.exports.queue_reset();
    const finished = waitForMessage(worker, 'Producer');
    const consume = async () => {
      const deadline = performance.now() + TIMEOUT_MS;
      let next = 0;
      while (next < COUNT) {
        if (cancelled) throw new Error('Consumer cancelled');
        // Bound each turn; an empty queue never makes queue_pop wait.
        for (let batch = 0; batch < 128 && next < COUNT; batch++) {
          const packet = main.exports.queue_pop();
          if (packet === -1n) break;
          assert.deepEqual(unpack(packet), [next, next ^ MASK]);
          next++;
        }
        if (performance.now() > deadline) throw new Error('Consumer timed out');
        await new Promise(resolve => setImmediate(resolve));
      }
    };
    worker.postMessage('start');
    // Attach both rejection handlers now, including while the consumer runs.
    const [, produced] = await Promise.all([consume(), finished]);
    assert.equal(produced, COUNT);
    assert.equal(main.exports.queue_pop(), -1n);
    console.log('PASS: shared Wasm memory; bounded full/empty handling; 100000 ordered two-word packets across workers.');
  } finally {
    cancelled = true;
    await worker.terminate();
  }
} else {
  const instance = await WebAssembly.instantiate(workerData.module, { env: { memory: workerData.memory } });
  parentPort.postMessage('ready');
  parentPort.once('message', async () => {
    let next = 0;
    while (next < COUNT) {
      for (let batch = 0; batch < 128 && next < COUNT; batch++) {
        if (!instance.exports.queue_push(next, next ^ MASK)) break;
        next++;
      }
      await new Promise(resolve => setImmediate(resolve));
    }
    parentPort.postMessage(next);
  });
}
