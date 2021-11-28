/* global csound:writable */

goog.provide("csound.utils.trim_null");

const trimNull = (a) => {
  const c = a.indexOf("\0");
  if (c > -1) {
    // eslint-disable-next-line unicorn/prefer-string-slice
    return a.substr(0, c);
  }
  return a;
};

csound.utils.trim_null = { trimNull };
