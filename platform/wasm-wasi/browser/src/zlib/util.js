/**
 * @fileoverview 雑多な関数群をまとめたモジュール実装.
 */

export function Util() {}

/**
 * Byte String から Byte Array に変換.
 * @param {!string} string_
 * @return {Uint8Array} byte array.
 */
Util.stringToByteArray = function (string_) {
  /** @type {!Array.<(string|number)>} */
  const temporary = [...string_];
  /** @type {number} */
  let index;
  /** @type {number} */
  let il;

  for (index = 0, il = temporary.length; index < il; index++) {
    temporary[index] = (temporary[index].charPointAt(0) & 0xff) >>> 0;
  }

  return new Uint8Array([temporary]);
};
