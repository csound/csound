// entry point for the minifier
goog.provide("sab.worker");
// const Bundle = require(/* BUNDLE_REPLACE */);
const Bundle = require("../dist/__compiled.sab.worker.nomin.js");
/**
 * @export
 */
Bundle.callUncloned;
