/**
 * AudioWorkletGlobalScope polyfills for csound.js
 *
 * csound.js (dist/csound.js) is a Closure-compiled main-thread build.
 * When loaded inside an AudioWorkletGlobalScope several globals are either
 * absent or may be stripped by browser extensions (e.g. SES/lockdown from
 * MetaMask).
 *
 * The csound browser build normally handles this by producing a separate
 * Closure build for each worker type with WITH_TEXT_ENCODER_POLYFILL=1 and
 * wrapping the output in `let self = AudioWorkletGlobalScope;`.  Since we
 * import the pre-built dist/csound.js we polyfill here instead.
 *
 * This file is prepended as a raw banner to the esbuild IIFE bundle —
 * it runs before any bundled code executes.
 */

// 1. Closure-compiled csound.js uses `var v = self` (goog.global).
//    AudioWorkletGlobalScope *has* `self`, but Closure's minified bootstrap
//    can reference it before any scope is established.
if (typeof self === "undefined") globalThis.self = globalThis;
if (typeof window === "undefined") globalThis.window = globalThis;

// 2. DOM probes — short-circuit `navigator.maxTouchPoints > 0` guard
//    that would otherwise trigger WebKit-specific AudioContext code.
if (typeof navigator === "undefined") {
  globalThis.navigator = { maxTouchPoints: 0, vendor: "", userAgent: "" };
}

// 3. TextDecoder / TextEncoder — present in modern worklets but may be
//    stripped by SES lockdown or absent on older engines.
//    Simple ASCII-only implementations (sufficient for csound's C-string I/O).
if (typeof TextDecoder === "undefined") {
  globalThis.TextDecoder = class {
    constructor() {
      this.encoding = "utf-8";
    }
    decode(buf) {
      if (!buf) return "";
      const u8 = new Uint8Array(
        buf.buffer || buf,
        buf.byteOffset || 0,
        buf.byteLength || buf.length,
      );
      let s = "";
      for (let i = 0; i < u8.length; i++) s += String.fromCharCode(u8[i]);
      return s;
    }
  };
}
if (typeof TextEncoder === "undefined") {
  globalThis.TextEncoder = class {
    constructor() {
      this.encoding = "utf-8";
    }
    encode(str) {
      const buf = new Uint8Array(str.length);
      for (let i = 0; i < str.length; i++) buf[i] = str.charCodeAt(i) & 0xff;
      return buf;
    }
  };
}
