/* eslint-disable unicorn/numeric-separators-style,camelcase,no-unused-expressions */
import { Util } from "./util";
/**
 * @fileoverview Adler32 checksum 実装.
 */

/**
 * Adler32 ハッシュ値の作成
 * @param {Uint8Array|string} array
 * @return {number} Adler32
 */
export function Adler32(array) {
  if (typeof array === "string") {
    array = Util.stringToByteArray(array);
  }
  return Adler32.update(1, array);
}

/**
 * Adler32 ハッシュ値の更新
 * @param {number} adler
 * @param {Uint8Array} array
 * @return {number} Adler32
 */
Adler32.update = function (adler, array) {
  /** @type {number} */
  let s1 = adler & 0xffff;
  /** @type {number} */
  let s2 = (adler >>> 16) & 0xffff;
  /** @type {number} array length */
  let length_ = array.length;
  /** @type {number} loop length (don't overflow) */
  let tlen;
  /** @type {number} array index */
  let index = 0;

  while (length_ > 0) {
    tlen = length_ > Adler32.OptimizationParameter ? Adler32.OptimizationParameter : length_;
    length_ -= tlen;
    do {
      s1 += array[index++];
      s2 += s1;
    } while (--tlen);

    s1 %= 65521;
    s2 %= 65521;
  }

  return ((s2 << 16) | s1) >>> 0;
};

/**
 * Adler32 最適化パラメータ
 * 現状では 1024 程度が最適.
 * @see http://jsperf.com/adler-32-simple-vs-optimized/3
 * @define {number}
 */
Adler32.OptimizationParameter = 1024;
