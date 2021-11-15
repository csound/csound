goog.provide("csound.utils.trim_null");

csound.utils.trim_null.trimNull = function (a) {
  const c = a.indexOf("\0");
  if (c > -1) {
    // eslint-disable-next-line unicorn/prefer-string-slice
    return a.substr(0, c);
  }
  return a;
};
