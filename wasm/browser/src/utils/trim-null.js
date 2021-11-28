export const trimNull = (a) => {
  const c = a.indexOf("\0");
  if (c > -1) {
    // eslint-disable-next-line unicorn/prefer-string-slice
    return a.substr(0, c);
  }
  return a;
};
