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

// Listen
console.log("Test server open on localhost:8080");
server.listen(8080);
