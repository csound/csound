/**
 * @fileoverview 雑多な関数群をまとめたモジュール実装.
 */

export function Util() {}

/**
 * Byte String から Byte Array に変換.
 * @param {!string} string_
 * @return {!Uint8Array} byte array.
 */
Util.stringToByteArray = function (string_) {
  const bytes = new Uint8Array(string_.length);
  for (let index = 0; index < string_.length; index++) {
    // eslint-disable-next-line unicorn/prefer-code-point -- Byte strings use one code unit per byte.
    bytes[index] = string_.charCodeAt(index) & 0xff;
  }
  return bytes;
};
