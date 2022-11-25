/** @license zlib.js 2012 - imaya [ https://github.com/imaya/zlib.js ] The MIT License */
/* eslint-disable unicorn/numeric-separators-style,camelcase,no-unused-expressions */

/**
 * build huffman table from length list.
 * @param {Uint8Array} lengths length list.
 * @return {Array.<number,number,number>} huffman table.
 */
export function Huffman(lengths) {
  /** @type {number} length list size. */
  const listSize = lengths.length;
  /** @type {number} max code length for table size. */
  let maxCodeLength = 0;
  /** @type {number} min code length for table size. */
  let minCodeLength = Number.POSITIVE_INFINITY;
  /** @type {number} bit length. */
  let bitLength;
  /** @type {number} huffman code. */
  let code;
  /**
   * @type {number} skip length for table filling.
   */
  let skip;
  /** @type {number} reversed code. */
  let reversed;
  /** @type {number} reverse temp. */
  let rtemp;
  /** @type {number} loop counter. */
  let index;
  /** @type {number} loop limit. */
  let il;
  /** @type {number} loop counter. */
  let index_;
  /** @type {number} table value. */
  let value;

  // Math.max は遅いので最長の値は for-loop で取得する
  for (index = 0, il = listSize; index < il; ++index) {
    if (lengths[index] > maxCodeLength) {
      maxCodeLength = lengths[index];
    }
    if (lengths[index] < minCodeLength) {
      minCodeLength = lengths[index];
    }
  }

  /** @type {number} table size. */
  const size = 1 << maxCodeLength;
  /** @type {Uint8Array|Uint16Array|Uint32Array} huffman code table. */
  const table = new Uint32Array(size);

  // ビット長の短い順からハフマン符号を割り当てる
  for (bitLength = 1, code = 0, skip = 2; bitLength <= maxCodeLength; ) {
    for (index = 0; index < listSize; ++index) {
      if (lengths[index] === bitLength) {
        // ビットオーダーが逆になるためビット長分並びを反転する
        for (reversed = 0, rtemp = code, index_ = 0; index_ < bitLength; ++index_) {
          reversed = (reversed << 1) | (rtemp & 1);
          rtemp >>= 1;
        }

        // 最大ビット長をもとにテーブルを作るため、
        // 最大ビット長以外では 0 / 1 どちらでも良い箇所ができる
        // そのどちらでも良い場所は同じ値で埋めることで
        // 本来のビット長以上のビット数取得しても問題が起こらないようにする
        value = (bitLength << 16) | index;
        for (index_ = reversed; index_ < size; index_ += skip) {
          table[index_] = value;
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
}
