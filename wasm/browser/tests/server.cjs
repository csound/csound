/*
 * Copyright (c) The Csound Developers
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const finalhandler = require("finalhandler");
const http = require("http");
const serveStatic = require("serve-static");

const serve = serveStatic(process.cwd() + "/tests", {
  index: "index.html",
  setHeaders: setHeaders,
});

function setHeaders(res, path) {
  res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
  res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
}

// Create server
const server = http.createServer(function onRequest(req, res) {
  serve(req, res, finalhandler(req, res));
});

const PORT = process.env.PORT || "8080";

// Listen
console.log(`Test server open on http://localhost:${PORT}`);
server.listen(PORT);
