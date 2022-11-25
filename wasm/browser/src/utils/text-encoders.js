/** @define {boolean} */
const WITH_TEXT_ENCODER_POLYFILL = goog.define("WITH_TEXT_ENCODER_POLYFILL", false);

/** @constructor */
function TextEncoderPoly() {
  this.encoding = "utf8";
  return this;
}

TextEncoderPoly.prototype.encode = function (string_) {
  if (typeof string_ !== "string") {
    throw new TypeError("passed argument must be of type string " + string_ + " " + typeof string_);
  }
  const binstr = unescape(encodeURIComponent(string_));
  const array = new Uint8Array(binstr.length);
  [...binstr].forEach(function (char, index) {
    array[index] = char.codePointAt(0);
  });
  return array;
};

/** @constructor */
function TextDecoderPoly() {
  this.encoding = "utf8";
  this.ignoreBOM = false;

  this.trimNull = (a) => {
    const c = a.indexOf("\0");
    if (c > -1) {
      return a.slice(0, Math.max(0, c));
    }
    return a;
  };

  this.decode = function (view, options) {
    if (view === undefined) {
      return "";
    }

    const stream = options !== undefined && "stream" in options ? options.stream : false;
    if (typeof stream !== "boolean") {
      throw new TypeError("stream option must be boolean");
    }

    if (ArrayBuffer.isView(view)) {
      const array = new Uint8Array(view.buffer, view.byteOffset, view.byteLength);
      const charArray = Array.from({ length: array.length });
      array.forEach(function (charcode, index) {
        charArray[index] = String.fromCodePoint(charcode);
      });
      return this.trimNull(charArray.join(""));
    } else {
      throw new TypeError("passed argument must be an array buffer view");
    }
  };
}

export const decoder = WITH_TEXT_ENCODER_POLYFILL ? new TextDecoderPoly() : new TextDecoder("utf8");

export const encoder = WITH_TEXT_ENCODER_POLYFILL ? new TextEncoderPoly() : new TextEncoder("utf8");

export const uint2String = (uint) => decoder.decode(uint);
