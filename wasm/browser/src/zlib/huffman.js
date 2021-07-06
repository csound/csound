/** @license zlib.js 2012 - imaya [ https://github.com/imaya/zlib.js ] The MIT License */
goog.provide("Zlib.Huffman");

goog.scope(function () {
  /**
   * build huffman table from length list.
   * @param {Uint8Array} lengths length list.
   * @return {Array.<number,number,number>} huffman table.
   */
  Zlib.Huffman.buildHuffmanTable = function (lengths) {
    /** @type {number} length list size. */
    var listSize = lengths.length;
    /** @type {number} max code length for table size. */
    var maxCodeLength = 0;
    /** @type {number} min code length for table size. */
    var minCodeLength = Number.POSITIVE_INFINITY;
    /** @type {number} table size. */
    var size;
    /** @type {Uint8Array|Uint16Array|Uint32Array} huffman code table. */
    var table;
    /** @type {number} bit length. */
    var bitLength;
    /** @type {number} huffman code. */
    var code;
    /**
     * @type {number} skip length for table filling.
     */
    var skip;
    /** @type {number} reversed code. */
    var reversed;
    /** @type {number} reverse temp. */
    var rtemp;
    /** @type {number} loop counter. */
    var i;
    /** @type {number} loop limit. */
    var il;
    /** @type {number} loop counter. */
    var j;
    /** @type {number} table value. */
    var value;

    // Math.max は遅いので最長の値は for-loop で取得する
    for (i = 0, il = listSize; i < il; ++i) {
      if (lengths[i] > maxCodeLength) {
        maxCodeLength = lengths[i];
      }
      if (lengths[i] < minCodeLength) {
        minCodeLength = lengths[i];
      }
    }

    size = 1 << maxCodeLength;
    table = new Uint32Array(size);

    // ビット長の短い順からハフマン符号を割り当てる
    for (bitLength = 1, code = 0, skip = 2; bitLength <= maxCodeLength; ) {
      for (i = 0; i < listSize; ++i) {
        if (lengths[i] === bitLength) {
          // ビットオーダーが逆になるためビット長分並びを反転する
          for (reversed = 0, rtemp = code, j = 0; j < bitLength; ++j) {
            reversed = (reversed << 1) | (rtemp & 1);
            rtemp >>= 1;
          }

          // 最大ビット長をもとにテーブルを作るため、
          // 最大ビット長以外では 0 / 1 どちらでも良い箇所ができる
          // そのどちらでも良い場所は同じ値で埋めることで
          // 本来のビット長以上のビット数取得しても問題が起こらないようにする
          value = (bitLength << 16) | i;
          for (j = reversed; j < size; j += skip) {
            table[j] = value;
          }

          ++code;
        }
      }

      // 次のビット長へ
      ++bitLength;
      code <<= 1;
      skip <<= 1;
    }

    return [table, maxCodeLength, minCodeLength];
  };

  // end of scope
});
