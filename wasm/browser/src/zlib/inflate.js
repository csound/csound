/* eslint-disable unicorn/numeric-separators-style,camelcase,no-unused-expressions */

import { CompressionMethod } from "./zlib";
import { RawInflate } from "./rawinflate";
import { Adler32 } from "./adler32";

/**
 * @constructor
 * @param {Uint8Array} input
 * @param {Object} [opt_parameters]
 *
 * opt_params は以下のプロパティを指定する事ができます。
 *   - index: input buffer の deflate コンテナの開始位置.
 *   - blockSize: バッファのブロックサイズ.
 *   - verify: 伸張が終わった後 adler-32 checksum の検証を行うか.
 *   - bufferType: Inflate.BufferType の値によってバッファの管理方法を指定する.
 *       Inflate.BufferType は RawInflate.BufferType のエイリアス.
 */
export function Inflate(input, opt_parameters) {
  /** @type {Uint8Array} */
  this.input = input;
  /** @type {number} */
  this.ip = 0;
  /** @type {RawInflate} */
  this.rawinflate;
  /** @type {(boolean|undefined)} verify flag. */
  this.verify;

  // option parameters
  if (opt_parameters || !(opt_parameters = {})) {
    if (opt_parameters.index) {
      this.ip = opt_parameters.index;
    }
    if (opt_parameters.verify) {
      this.verify = opt_parameters.verify;
    }
  }

  // Compression Method and Flags
  /** @type {number} */
  const cmf = input[this.ip++];
  /** @type {number} */
  const flg = input[this.ip++];

  // compression method
  switch (cmf & 0x0f) {
    case CompressionMethod.DEFLATE: {
      this.method = CompressionMethod.DEFLATE;
      break;
    }
    default: {
      throw new Error("unsupported compression method");
    }
  }

  // fcheck
  if (((cmf << 8) + flg) % 31 !== 0) {
    throw new Error("invalid fcheck flag:" + (((cmf << 8) + flg) % 31));
  }

  // fdict (not supported)
  if (flg & 0x20) {
    throw new Error("fdict flag is not supported");
  }

  // RawInflate
  this.rawinflate = new RawInflate(input, {
    index: this.ip,
    bufferSize: opt_parameters.bufferSize,
    bufferType: opt_parameters.bufferType,
    resize: opt_parameters.resize,
  });
}

/**
 * @enum {number}
 */
Inflate.BufferType = RawInflate.BufferType;

/**
 * decompress.
 * @return {Uint8Array} inflated buffer.
 */
Inflate.prototype.decompress = function () {
  /** @type {Uint8Array} input buffer. */
  const input = this.input;
  /** @type {number} adler-32 checksum */
  let adler32;
  /** @type {Uint8Array} inflated buffer. */
  const buffer = this.rawinflate.decompress();
  this.ip = this.rawinflate.ip;

  // verify adler-32
  if (this.verify) {
    adler32 =
      ((input[this.ip++] << 24) |
        (input[this.ip++] << 16) |
        (input[this.ip++] << 8) |
        input[this.ip++]) >>>
      0;

    if (adler32 !== Adler32(buffer)) {
      throw new Error("invalid adler-32 checksum");
    }
  }

  return buffer;
};
