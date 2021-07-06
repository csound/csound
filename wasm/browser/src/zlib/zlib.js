/**
 * @fileoverview Zlib namespace. Zlib の仕様に準拠した圧縮は Zlib.Deflate で実装
 * されている. これは Inflate との共存を考慮している為.
 */

goog.provide("Zlib");

//-----------------------------------------------------------------------------

goog.scope(function () {
  /**
   * Compression Method
   * @enum {number}
   */
  Zlib.CompressionMethod = {
    DEFLATE: 8,
    RESERVED: 15,
  };

  // end of scope
});
