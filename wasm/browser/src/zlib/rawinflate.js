/* eslint-disable unicorn/numeric-separators-style,camelcase,no-unused-expressions,unicorn/prevent-abbreviations */

import { Huffman } from "./huffman";

/** @define {number} buffer block size. */
const ZLIB_RAW_INFLATE_BUFFER_SIZE = 0x8000; // [ 0x8000 >= ZLIB_BUFFER_BLOCK_SIZE ]

/**
 * @constructor
 * @param {!(Uint8Array)} input
 * @param {Object} [opt_parameters]
 *
 * opt_params は以下のプロパティを指定する事ができます。
 *   - index: input buffer の deflate コンテナの開始位置.
 *   - blockSize: バッファのブロックサイズ.
 *   - bufferType: RawInflate.BufferType の値によってバッファの管理方法を指定する.
 *   - resize: 確保したバッファが実際の大きさより大きかった場合に切り詰める.
 */
export function RawInflate(input, opt_parameters) {
  /** @type {!(Uint8Array)} inflated buffer */
  this.buffer;
  /** @type {!Array.<(Uint8Array)>} */
  this.blocks = [];
  /** @type {number} block size. */
  this.bufferSize = ZLIB_RAW_INFLATE_BUFFER_SIZE;
  /** @type {!number} total output buffer pointer. */
  this.totalpos = 0;
  /** @type {!number} input buffer pointer. */
  this.ip = 0;
  /** @type {!number} bit stream reader buffer. */
  this.bitsbuf = 0;
  /** @type {!number} bit stream reader buffer size. */
  this.bitsbuflen = 0;
  /** @type {!(Uint8Array)} input buffer. */
  this.input = new Uint8Array(input);
  /** @type {!(Uint8Array)} output buffer. */
  this.output;
  /** @type {!number} output buffer pointer. */
  this.op;
  /** @type {boolean} is final block flag. */
  this.bfinal = false;
  /** @type {RawInflate.BufferType} buffer management. */
  this.bufferType = RawInflate.BufferType.ADAPTIVE;
  /** @type {boolean} resize flag for memory size optimization. */
  this.resize = false;

  // option parameters
  if (opt_parameters || !(opt_parameters = {})) {
    if (opt_parameters.index) {
      this.ip = opt_parameters.index;
    }
    if (opt_parameters.bufferSize) {
      this.bufferSize = opt_parameters.bufferSize;
    }
    if (opt_parameters.bufferType) {
      this.bufferType = opt_parameters.bufferType;
    }
    if (opt_parameters.resize) {
      this.resize = opt_parameters.resize;
    }
  }

  // initialize
  switch (this.bufferType) {
    case RawInflate.BufferType.BLOCK: {
      this.op = RawInflate.MaxBackwardLength;
      this.output = new Uint8Array(
        RawInflate.MaxBackwardLength + this.bufferSize + RawInflate.MaxCopyLength,
      );
      break;
    }
    case RawInflate.BufferType.ADAPTIVE: {
      this.op = 0;
      this.output = new Uint8Array(this.bufferSize);
      break;
    }
    default: {
      throw new Error("invalid inflate mode");
    }
  }
}

/**
 * @enum {number}
 */
RawInflate.BufferType = {
  BLOCK: 0,
  ADAPTIVE: 1,
};

/**
 * decompress.
 * @return {!(Uint8Array)} inflated buffer.
 */
RawInflate.prototype.decompress = function () {
  while (!this.bfinal) {
    this.parseBlock();
  }

  switch (this.bufferType) {
    case RawInflate.BufferType.BLOCK: {
      return this.concatBufferBlock();
    }
    case RawInflate.BufferType.ADAPTIVE: {
      return this.concatBufferDynamic();
    }
    default: {
      throw new Error("invalid inflate mode");
    }
  }
};

/**
 * @const
 * @type {number} max backward length for LZ77.
 */
RawInflate.MaxBackwardLength = 32768;

/**
 * @const
 * @type {number} max copy length for LZ77.
 */
RawInflate.MaxCopyLength = 258;

/**
 * huffman order
 * @const
 * @type {!(Uint8Array)}
 */
RawInflate.Order = (function (table) {
  return new Uint16Array(table);
})([16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]);

/**
 * huffman length code table.
 * @const
 * @type {!(Uint16Array)}
 */
RawInflate.LengthCodeTable = (function (table) {
  return new Uint16Array(table);
})([
  0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000d, 0x000f, 0x0011,
  0x0013, 0x0017, 0x001b, 0x001f, 0x0023, 0x002b, 0x0033, 0x003b, 0x0043, 0x0053, 0x0063, 0x0073,
  0x0083, 0x00a3, 0x00c3, 0x00e3, 0x0102, 0x0102, 0x0102,
]);

/**
 * huffman length extra-bits table.
 * @const
 * @type {!(Uint8Array)}
 */
RawInflate.LengthExtraTable = (function (table) {
  return new Uint8Array(table);
})([0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0]);

/**
 * huffman dist code table.
 * @const
 * @type {!(Uint16Array)}
 */
RawInflate.DistCodeTable = (function (table) {
  return new Uint16Array(table);
})([
  0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d, 0x0011, 0x0019, 0x0021, 0x0031,
  0x0041, 0x0061, 0x0081, 0x00c1, 0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01,
  0x1001, 0x1801, 0x2001, 0x3001, 0x4001, 0x6001,
]);

/**
 * huffman dist extra-bits table.
 * @const
 * @type {!(Uint8Array)}
 */
RawInflate.DistExtraTable = (function (table) {
  return new Uint8Array(table);
})([
  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
]);

/**
 * fixed huffman length code table
 * @const
 * @type {!Array}
 */
RawInflate.FixedLiteralLengthTable = (function (table) {
  return table;
})(
  (function () {
    const lengths = new Uint8Array(288);
    let index, il;

    for (index = 0, il = lengths.length; index < il; ++index) {
      lengths[index] = index <= 143 ? 8 : index <= 255 ? 9 : index <= 279 ? 7 : 8;
    }

    return Huffman(lengths);
  })(),
);

/**
 * fixed huffman distance code table
 * @const
 * @type {!Array}
 */
RawInflate.FixedDistanceTable = (function (table) {
  return table;
})(
  (function () {
    const lengths = new Uint8Array(30);
    let index, il;

    for (index = 0, il = lengths.length; index < il; ++index) {
      lengths[index] = 5;
    }

    return Huffman(lengths);
  })(),
);

/**
 * parse deflated block.
 */
RawInflate.prototype.parseBlock = function () {
  /** @type {number} header */
  let hdr = this.readBits(3);

  // BFINAL
  if (hdr & 0x1) {
    this.bfinal = true;
  }

  // BTYPE
  hdr >>>= 1;
  switch (hdr) {
    // uncompressed
    case 0: {
      this.parseUncompressedBlock();
      break;
    }
    // fixed huffman
    case 1: {
      this.parseFixedHuffmanBlock();
      break;
    }
    // dynamic huffman
    case 2: {
      this.parseDynamicHuffmanBlock();
      break;
    }
    // reserved or other
    default: {
      throw new Error("unknown BTYPE: " + hdr);
    }
  }
};

/**
 * read inflate bits
 * @param {number} length bits length.
 * @return {number} read bits.
 */
RawInflate.prototype.readBits = function (length) {
  let bitsbuf = this.bitsbuf;
  let bitsbuflen = this.bitsbuflen;
  const input = this.input;
  let ip = this.ip;

  /** @type {number} */
  const inputLength = input.length;

  if (ip + ((length - bitsbuflen + 7) >> 3) >= inputLength) {
    throw new Error("input buffer is broken");
  }

  // not enough buffer
  while (bitsbuflen < length) {
    bitsbuf |= input[ip++] << bitsbuflen;
    bitsbuflen += 8;
  }

  /** @type {number} input and output byte. */
  const octet = bitsbuf & /* MASK */ ((1 << length) - 1);
  bitsbuf >>>= length;
  bitsbuflen -= length;

  this.bitsbuf = bitsbuf;
  this.bitsbuflen = bitsbuflen;
  this.ip = ip;

  return octet;
};

/**
 * read huffman code using table
 * @param {Array.<number>|Uint8Array|Uint16Array|null} table huffman code table.
 * @return {number} huffman code.
 */
RawInflate.prototype.readCodeByTable = function (table) {
  let bitsbuf = this.bitsbuf;
  let bitsbuflen = this.bitsbuflen;
  const input = this.input;
  let ip = this.ip;

  /** @type {number} */
  const inputLength = input.length;
  /** @type {!(Uint8Array)} huffman code table */
  const codeTable = table[0];
  /** @type {number} */
  const maxCodeLength = table[1];

  // not enough buffer
  while (bitsbuflen < maxCodeLength) {
    if (ip >= inputLength) {
      break;
    }
    bitsbuf |= input[ip++] << bitsbuflen;
    bitsbuflen += 8;
  }

  // read max length
  /** @type {number} code length & code (16bit, 16bit) */
  const codeWithLength = codeTable[bitsbuf & ((1 << maxCodeLength) - 1)];
  /** @type {number} code bits length */
  const codeLength = codeWithLength >>> 16;

  if (codeLength > bitsbuflen) {
    throw new Error("invalid code length: " + codeLength);
  }

  this.bitsbuf = bitsbuf >> codeLength;
  this.bitsbuflen = bitsbuflen - codeLength;
  this.ip = ip;

  return codeWithLength & 0xffff;
};

/**
 * parse uncompressed block.
 */
RawInflate.prototype.parseUncompressedBlock = function () {
  const input = this.input;
  let ip = this.ip;
  let output = this.output;
  let op = this.op;

  /** @type {number} */
  const inputLength = input.length;
  /** @type {number} block length */
  let length_;
  /** @type {number} output buffer length */
  const olength = output.length;
  /** @type {number} copy counter */
  let preCopy;

  // skip buffered header bits
  this.bitsbuf = 0;
  this.bitsbuflen = 0;

  // len
  if (ip + 1 >= inputLength) {
    throw new Error("invalid uncompressed block header: LEN");
  }
  length_ = input[ip++] | (input[ip++] << 8);

  // nlen
  if (ip + 1 >= inputLength) {
    throw new Error("invalid uncompressed block header: NLEN");
  }

  /** @type {number} number for check block length */
  const nlen = input[ip++] | (input[ip++] << 8);

  // check len & nlen
  if (length_ === ~nlen) {
    throw new Error("invalid uncompressed block header: length verify");
  }

  // check size
  if (ip + length_ > input.length) {
    throw new Error("input buffer is broken");
  }

  // expand buffer
  switch (this.bufferType) {
    case RawInflate.BufferType.BLOCK: {
      // pre copy
      while (op + length_ > output.length) {
        preCopy = olength - op;
        length_ -= preCopy;
        output.set(input.subarray(ip, ip + preCopy), op);
        op += preCopy;
        ip += preCopy;

        this.op = op;
        output = this.expandBufferBlock();
        op = this.op;
      }
      break;
    }
    case RawInflate.BufferType.ADAPTIVE: {
      while (op + length_ > output.length) {
        output = this.expandBufferAdaptive({ fixRatio: 2 });
      }
      break;
    }
    default: {
      throw new Error("invalid inflate mode");
    }
  }

  // copy
  output.set(input.subarray(ip, ip + length_), op);
  op += length_;
  ip += length_;

  this.ip = ip;
  this.op = op;
  this.output = output;
};

/**
 * parse fixed huffman block.
 */
RawInflate.prototype.parseFixedHuffmanBlock = function () {
  switch (this.bufferType) {
    case RawInflate.BufferType.ADAPTIVE: {
      this.decodeHuffmanAdaptive(RawInflate.FixedLiteralLengthTable, RawInflate.FixedDistanceTable);
      break;
    }
    case RawInflate.BufferType.BLOCK: {
      this.decodeHuffmanBlock(RawInflate.FixedLiteralLengthTable, RawInflate.FixedDistanceTable);
      break;
    }
    default: {
      throw new Error("invalid inflate mode");
    }
  }
};

/**
 * parse dynamic huffman block.
 */
RawInflate.prototype.parseDynamicHuffmanBlock = function () {
  /** @type {number} number of literal and length codes. */
  const hlit = this.readBits(5) + 257;
  /** @type {number} number of distance codes. */
  const hdist = this.readBits(5) + 1;
  /** @type {number} number of code lengths. */
  const hclen = this.readBits(4) + 4;
  /** @type {Uint8Array} code lengths. */
  const codeLengths = new Uint8Array(RawInflate.Order.length);
  /** @type {number} */
  let code;
  /** @type {number} */
  let previous;
  /** @type {number} */
  let repeat;
  /** @type {number} loop counter. */
  let index;
  /** @type {number} loop limit. */
  let il;

  // decode code lengths
  for (index = 0; index < hclen; ++index) {
    codeLengths[RawInflate.Order[index]] = this.readBits(3);
  }

  // decode length table
  /** @type {Array.<number,number,number>} code lengths table. */
  const codeLengthsTable = Huffman(codeLengths);
  /** @type {Uint8Array} code length table. */
  const lengthTable = new Uint8Array(hlit + hdist);
  for (index = 0, il = hlit + hdist; index < il; ) {
    code = this.readCodeByTable(codeLengthsTable);
    switch (code) {
      case 16: {
        repeat = 3 + this.readBits(2);
        while (repeat--) {
          lengthTable[index++] = previous;
        }
        break;
      }
      case 17: {
        repeat = 3 + this.readBits(3);
        while (repeat--) {
          lengthTable[index++] = 0;
        }
        previous = 0;
        break;
      }
      case 18: {
        repeat = 11 + this.readBits(7);
        while (repeat--) {
          lengthTable[index++] = 0;
        }
        previous = 0;
        break;
      }
      default: {
        lengthTable[index++] = code;
        previous = code;
        break;
      }
    }
  }

  /** @type {Array.<number>|Uint8Array|null} literal and length code table. */
  const litlenTable = Huffman(lengthTable.subarray(0, hlit));
  /** @type {Array.<number>|Uint8Array} distance code table. */
  const distTable = Huffman(lengthTable.subarray(hlit));

  switch (this.bufferType) {
    case RawInflate.BufferType.ADAPTIVE: {
      this.decodeHuffmanAdaptive(litlenTable, distTable);
      break;
    }
    case RawInflate.BufferType.BLOCK: {
      this.decodeHuffmanBlock(litlenTable, distTable);
      break;
    }
    default: {
      throw new Error("invalid inflate mode");
    }
  }
};

/**
 * decode huffman code
 * @param {Array.<number>|Uint8Array|Uint16Array} litlen literal and length code table.
 * @param {Array.<number>|Uint8Array} dist distination code table.
 */
RawInflate.prototype.decodeHuffmanBlock = function (litlen, dist) {
  let output = this.output;
  let op = this.op;

  this.currentLitlenTable = litlen;

  /** @type {number} output position limit. */
  const olength = output.length - RawInflate.MaxCopyLength;
  /** @type {number} huffman code. */
  let code;
  /** @type {number} table index. */
  let ti;
  /** @type {number} huffman code distination. */
  let codeDist;
  /** @type {number} huffman code length. */
  let codeLength;

  const lengthCodeTable = RawInflate.LengthCodeTable;
  const lengthExtraTable = RawInflate.LengthExtraTable;
  const distCodeTable = RawInflate.DistCodeTable;
  const distExtraTable = RawInflate.DistExtraTable;

  while ((code = this.readCodeByTable(litlen)) !== 256) {
    // literal
    if (code < 256) {
      if (op >= olength) {
        this.op = op;
        output = this.expandBufferBlock();
        op = this.op;
      }
      output[op++] = code;

      continue;
    }

    // length code
    ti = code - 257;
    codeLength = lengthCodeTable[ti];
    if (lengthExtraTable[ti] > 0) {
      codeLength += this.readBits(lengthExtraTable[ti]);
    }

    // dist code
    code = this.readCodeByTable(dist);
    codeDist = distCodeTable[code];
    if (distExtraTable[code] > 0) {
      codeDist += this.readBits(distExtraTable[code]);
    }

    // lz77 decode
    if (op >= olength) {
      this.op = op;
      output = this.expandBufferBlock();
      op = this.op;
    }
    while (codeLength--) {
      output[op] = output[op++ - codeDist];
    }
  }

  while (this.bitsbuflen >= 8) {
    this.bitsbuflen -= 8;
    this.ip--;
  }
  this.op = op;
};

/**
 * decode huffman code (adaptive)
 * @param {Array.<number>|Uint8Array|Uint16Array} litlen literal and length code table.
 * @param {Array.<number>|Uint8Array} dist distination code table.
 */
RawInflate.prototype.decodeHuffmanAdaptive = function (litlen, dist) {
  let output = this.output;
  let op = this.op;

  this.currentLitlenTable = litlen;

  /** @type {number} output position limit. */
  let olength = output.length;
  /** @type {number} huffman code. */
  let code;
  /** @type {number} table index. */
  let ti;
  /** @type {number} huffman code distination. */
  let codeDist;
  /** @type {number} huffman code length. */
  let codeLength;

  const lengthCodeTable = RawInflate.LengthCodeTable;
  const lengthExtraTable = RawInflate.LengthExtraTable;
  const distCodeTable = RawInflate.DistCodeTable;
  const distExtraTable = RawInflate.DistExtraTable;

  while ((code = this.readCodeByTable(litlen)) !== 256) {
    // literal
    if (code < 256) {
      if (op >= olength) {
        output = this.expandBufferAdaptive();
        olength = output.length;
      }
      output[op++] = code;

      continue;
    }

    // length code
    ti = code - 257;
    codeLength = lengthCodeTable[ti];
    if (lengthExtraTable[ti] > 0) {
      codeLength += this.readBits(lengthExtraTable[ti]);
    }

    // dist code
    code = this.readCodeByTable(dist);
    codeDist = distCodeTable[code];
    if (distExtraTable[code] > 0) {
      codeDist += this.readBits(distExtraTable[code]);
    }

    // lz77 decode
    if (op + codeLength > olength) {
      output = this.expandBufferAdaptive();
      olength = output.length;
    }
    while (codeLength--) {
      output[op] = output[op++ - codeDist];
    }
  }

  while (this.bitsbuflen >= 8) {
    this.bitsbuflen -= 8;
    this.ip--;
  }
  this.op = op;
};

/**
 * expand output buffer.
 * @param {Object} [opt_parameter]
 * @return {!(Uint8Array)} output buffer.
 */
RawInflate.prototype.expandBufferBlock = function (opt_parameter) {
  /** @type {!(Uint8Array)} store buffer. */
  const buffer = new Uint8Array(this.op - RawInflate.MaxBackwardLength);
  /** @type {number} backward base point */
  const backward = this.op - RawInflate.MaxBackwardLength;

  const output = this.output;

  // copy to output buffer
  buffer.set(output.subarray(RawInflate.MaxBackwardLength, buffer.length));

  this.blocks.push(buffer);
  this.totalpos += buffer.length;

  // copy to backward buffer
  output.set(output.subarray(backward, backward + RawInflate.MaxBackwardLength));

  this.op = RawInflate.MaxBackwardLength;

  return output;
};

/**
 * expand output buffer. (adaptive)
 * @param {Object=} opt_parameter
 * @return {!(Uint8Array)} output buffer pointer.
 */
RawInflate.prototype.expandBufferAdaptive = function (opt_parameter) {
  /** @type {number} expantion ratio. */
  let ratio = Math.trunc(this.input.length / this.ip + 1);
  /** @type {number} maximum number of huffman code. */
  let maxHuffCode;
  /** @type {number} new output buffer size. */
  let newSize;
  /** @type {number} max inflate size. */
  let maxInflateSize;

  const input = this.input;
  const output = this.output;

  if (opt_parameter) {
    if (typeof opt_parameter.fixRatio === "number") {
      ratio = opt_parameter.fixRatio;
    }
    if (typeof opt_parameter.addRatio === "number") {
      ratio += opt_parameter.addRatio;
    }
  }

  // calculate new buffer size
  if (ratio < 2) {
    maxHuffCode = (input.length - this.ip) / this.currentLitlenTable[2];
    maxInflateSize = Math.trunc((maxHuffCode / 2) * 258);
    newSize = maxInflateSize < output.length ? output.length + maxInflateSize : output.length << 1;
  } else {
    newSize = output.length * ratio;
  }

  // buffer expantion
  /** @type {!(Uint8Array)} store buffer. */
  const buffer = new Uint8Array(newSize);
  buffer.set(output);

  this.output = buffer;

  return this.output;
};

/**
 * concat output buffer.
 * @return {!(Uint8Array)} output buffer.
 */
RawInflate.prototype.concatBufferBlock = function () {
  /** @type {number} buffer pointer. */
  let pos = 0;
  /** @type {number} buffer pointer. */
  const limit = this.totalpos + (this.op - RawInflate.MaxBackwardLength);
  /** @type {!(Uint8Array)} output block array. */
  const output = this.output;
  /** @type {!Array} blocks array. */
  const blocks = this.blocks;
  /** @type {!(Uint8Array)} output block array. */
  let block;
  /** @type {!(Uint8Array)} output buffer. */
  const buffer = new Uint8Array(limit);
  /** @type {number} loop counter. */
  let index;
  /** @type {number} loop limiter. */
  let il;
  /** @type {number} loop counter. */
  let index_;
  /** @type {number} loop limiter. */
  let jl;

  // single buffer
  if (blocks.length === 0) {
    return this.output.subarray(RawInflate.MaxBackwardLength, this.op);
  }

  // copy to buffer
  for (index = 0, il = blocks.length; index < il; ++index) {
    block = blocks[index];
    for (index_ = 0, jl = block.length; index_ < jl; ++index_) {
      buffer[pos++] = block[index_];
    }
  }

  // current buffer
  for (index = RawInflate.MaxBackwardLength, il = this.op; index < il; ++index) {
    buffer[pos++] = output[index];
  }

  this.blocks = [];
  this.buffer = buffer;

  return this.buffer;
};

/**
 * concat output buffer. (dynamic)
 * @return {!(Uint8Array)} output buffer.
 */
RawInflate.prototype.concatBufferDynamic = function () {
  /** @type {Uint8Array} output buffer. */
  let buffer;
  const op = this.op;
  if (this.resize) {
    buffer = new Uint8Array(op);
    buffer.set(this.output.subarray(0, op));
  } else {
    buffer = this.output.subarray(0, op);
  }

  this.buffer = buffer;

  return this.buffer;
};
