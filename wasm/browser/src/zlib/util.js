/**
 * @fileoverview 雑多な関数群をまとめたモジュール実装.
 */
goog.provide("Zlib.Util");

goog.scope(function () {
  /**
   * Byte String から Byte Array に変換.
   * @param {!string} str byte string.
   * @return {!Array.<number>} byte array.
   */
  Zlib.Util.stringToByteArray = function (str) {
    /** @type {!Array.<(string|number)>} */
    var tmp = str.split("");
    /** @type {number} */
    var i;
    /** @type {number} */
    var il;

    for (i = 0, il = tmp.length; i < il; i++) {
      tmp[i] = (tmp[i].charCodeAt(0) & 0xff) >>> 0;
    }

    return tmp;
  };

  // end of scope
});
