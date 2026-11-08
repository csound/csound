/* eslint-disable unicorn/prefer-global-this */

// Keep a fallback for browsers that predate globalThis.
export const getGlobalScope = () => {
  if (typeof globalThis !== "undefined") {
    return globalThis;
  }
  if (typeof self !== "undefined") {
    return self;
  }
  if (typeof window !== "undefined") {
    return window;
  }
};

/* eslint-enable unicorn/prefer-global-this */
