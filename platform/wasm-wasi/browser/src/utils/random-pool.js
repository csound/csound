/* eslint-disable object-shorthand -- Message keys cross bundle boundaries. */
// AudioWorklet has no Web Crypto API. Its main thread supplies fresh entropy;
// each byte is consumed once. Refills run between audio callbacks, never block
// the render thread, and never fall back to a non-cryptographic generator.
export function randomBytes() {
  const bytes = new Uint8Array(131072);
  crypto.getRandomValues(bytes.subarray(0, 65536));
  crypto.getRandomValues(bytes.subarray(65536));
  return bytes;
}

// For applications hosting libcsound directly in their own AudioWorklet.
export function serveRandomBytes(port) {
  port.addEventListener("message", ({ data }) => {
    if (data?.["type"] !== "csound:random-request") return;
    const bytes = randomBytes();
    // prettier-ignore
    port.postMessage({ "type": "csound:random-bytes", "bytes": bytes }, [bytes.buffer]);
  });
  port.start();
}

export function workletRandomValues(bytes, port) {
  let resolveRefill;
  port.addEventListener("message", ({ data }) => {
    if (data?.["type"] !== "csound:random-bytes" || !resolveRefill) return;
    const resolve = resolveRefill;
    resolveRefill = undefined;
    resolve(data["bytes"]);
  });
  port.start();
  const pool = new RandomPool(
    bytes,
    () =>
      new Promise((resolve) => {
        resolveRefill = resolve;
        // prettier-ignore
        port.postMessage({ "type": "csound:random-request" });
      }),
  );
  return (target) => pool.fill(target);
}

export class RandomPool {
  constructor(bytes, refill) {
    this.chunks = [bytes];
    this.available = bytes.length;
    this.refill = refill;
    this.pending = false;
    this.error = undefined;
  }

  fill(target) {
    if (target.length > this.available)
      throw this.error || new Error("AudioWorklet exhausted its secure random-byte pool");
    let offset = 0;
    while (offset < target.length) {
      const chunk = this.chunks[0];
      const length = Math.min(chunk.length, target.length - offset);
      target.set(chunk.subarray(0, length), offset);
      chunk.fill(0, 0, length);
      if (length === chunk.length) this.chunks.shift();
      else this.chunks[0] = chunk.subarray(length);
      offset += length;
      this.available -= length;
    }
    if (this.available < 65536 && !this.pending) {
      this.pending = true;
      Promise.resolve()
        .then(() => this.refill())
        .then((bytes) => {
          this.chunks.push(bytes);
          this.available += bytes.length;
        })
        .catch((error) => {
          this.error = error;
        })
        .finally(() => {
          this.pending = false;
        });
    }
    return target;
  }
}
