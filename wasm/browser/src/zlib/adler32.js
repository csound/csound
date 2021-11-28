/* eslint-disable unicorn/numeric-separators-style,camelcase,no-unused-expressions */
/**
 * @fileoverview Adler32 checksum 実装.
 */
goog.provide("Zlib.Adler32");

goog.require("Zlib.Util");

goog.scope(function () {
  /**
   * Adler32 ハッシュ値の作成
   * @param {!(Array|Uint8Array|string)} array 算出に使用する byte array.
   * @return {number} Adler32 ハッシュ値.
   */
  Zlib.Adler32 = function (array) {
    if (typeof array === "string") {
      array = Zlib.Util.stringToByteArray(array);
    }
    return Zlib.Adler32.update(1, array);
  };

  /**
   * Adler32 ハッシュ値の更新
   * @param {number} adler 現在のハッシュ値.
   * @param {!(Array|Uint8Array)} array 更新に使用する byte array.
   * @return {number} Adler32 ハッシュ値.
   */
  Zlib.Adler32.update = function (adler, array) {
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
      tlen =
        length_ > Zlib.Adler32.OptimizationParameter ? Zlib.Adler32.OptimizationParameter : length_;
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
  Zlib.Adler32.OptimizationParameter = 1024;

  // end of scope
});
