/**
 * @fileoverview Zlib namespace. Zlib の仕様に準拠した圧縮は Deflate で実装
 * されている. これは Inflate との共存を考慮している為.
 */

/**
 * Compression Method
 * @enum {number}
 */
export const CompressionMethod = {
  DEFLATE: 8,
  RESERVED: 15,
};
