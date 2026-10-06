# Third-Party Licenses

This document lists third-party code that is vendored (embedded) directly into the
`@csound/browser` source tree, along with key runtime dependencies and their licenses.

---

## Vendored Source Files

These files are copied directly into this repository and are **not** installed via npm.

### Comlink

- **File:** `src/utils/comlink.js`
- **License:** Apache License 2.0
- **Copyright:** Copyright 2017 Google Inc.
- **Source:** https://github.com/GoogleChromeLabs/comlink

### zlib.js

- **Files:** `src/zlib/huffman.js`, `src/zlib/rawinflate.js`, `src/zlib/inflate.js`,
  `src/zlib/adler32.js`, `src/zlib/util.js`, `src/zlib/zlib.js`
- **License:** MIT
- **Copyright:** Copyright 2012 imaya
- **Source:** https://github.com/imaya/zlib.js

### wasi-js constants

- **File:** `src/filesystem/constants.js`
- **License:** MIT
- **Copyright:** Copyright 2017 Syrus (me@syrusakbary.com), Copyright 2019 Gus Caplan
- **Source:** https://github.com/wasmerio/wasmer-js (originally https://github.com/devsnek/node-wasi)

---

## Runtime npm Dependencies

`@bjorn3/browser_wasi_shim` (MIT OR Apache-2.0,
https://github.com/bjorn3/browser_wasi_shim) supplies the WASI command host.
The build bundles it into the browser output. Its command binaries remain
separate files supplied by the application.

These packages are installed via npm and are **not** bundled into the published package
output — they are resolved by the end user's bundler or loaded from npm at build time.

| Package | License | Repository |
|---------|---------|------------|
| `eventemitter3` | MIT | https://github.com/primus/eventemitter3 |
| `pako` | MIT | https://github.com/nodeca/pako |
| `ramda` | MIT | https://github.com/ramda/ramda |
| `standardized-audio-context` | MIT | https://github.com/chrisguttandin/standardized-audio-context |
| `text-encoding-shim` | Unlicense / Apache-2.0 | https://github.com/nicktindall/cyclon.p2p-common |
| `unmute-ios-audio` | MIT | https://github.com/feross/unmute-ios-audio |
| `web-midi-api` | MIT | https://github.com/notthetup/web-midi-api |

---

## Build-Time-Only Dependencies

These packages are used during the build process only and are **not** included in the
published package.

| Package | License | Notes |
|---------|---------|-------|
| `google-closure-compiler` | Apache-2.0 | Build optimizer |
| `google-closure-library` | Apache-2.0 | Build support library |
| `rimraf` | ISC | File cleanup |
| `prettier` | MIT | Code formatting |
| `eslint` (and plugins) | MIT | Linting |
| `mocha` | MIT | Test runner |
| `chai` | MIT | Test assertions |
| `sinon` | BSD-3-Clause | Test mocking |
| `selenium-webdriver` | Apache-2.0 | Browser test automation |
| `jsdoc` / `jsdoc-to-markdown` | Apache-2.0 | Documentation generation |

## browser_wasi_shim MIT license

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without
limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
