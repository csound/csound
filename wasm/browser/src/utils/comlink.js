// Copyright 2017 Google Inc.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// VENDORED DEPENDENCY
// sadly, comlink doesn't work with advanced google-closure compilation
// see: https://github.com/GoogleChromeLabs/comlink

const e = Symbol("Comlink.proxy");
const t = Symbol("Comlink.endpoint");
let n = Symbol("Comlink.releaseProxy");
const r = Symbol("Comlink.thrown");
const a = (e) => ("object" == typeof e && null !== e) || "function" == typeof e;
const s = new Map([
  [
    "proxy",
    {
      canHandle: (t) => a(t) && t[e],
      serialize(e) {
        const { port1: t, port2: n } = new MessageChannel();
        return o(e, t), [n, [n]];
      },
      deserialize: (e) => (e.start(), c(e)),
    },
  ],
  [
    "throw",
    {
      canHandle: (e) => a(e) && r in e,
      serialize({ value: e }) {
        let t;
        return (
          (t =
            e instanceof Error
              ? { isError: !0, value: { message: e.message, name: e.name, stack: e.stack } }
              : { isError: !1, value: e }),
          [t, []]
        );
      },
      deserialize(e) {
        if (e.isError) throw Object.assign(new Error(e.value.message), e.value);
        throw e.value;
      },
    },
  ],
]);
function o(e, t = self) {
  t.addEventListener("message", function n(a) {
    if (!a || !a.data) return;
    const argumentList = a["data"]["argumentList"];
    const { id: s, type: c, path: u } = Object.assign({ path: [] }, a["data"]);
    const l = (argumentList || []).map(E);

    let p;
    try {
      const t = u.slice(0, -1).reduce((e, t) => e[t], e),
        n = u.reduce((e, t) => e[t], e);
      switch (c) {
        case "GET":
          p = n;
          break;
        case "SET":
          (t[u.slice(-1)[0]] = E(a.data.value)), (p = !0);
          break;
        case "APPLY":
          console.log("N.apply", n, t, l);
          p = n.apply(t, l);
          break;
        case "CONSTRUCT":
          p = d(new n(...l));
          break;
        case "ENDPOINT":
          {
            const { port1: t, port2: n } = new MessageChannel();
            o(e, n), (p = m(t, [t]));
          }
          break;
        case "RELEASE":
          p = void 0;
          break;
        default:
          return;
      }
    } catch (e) {
      p = { value: e, [r]: 0 };
    }
    Promise.resolve(p)
      .catch((e) => ({ value: e, [r]: 0 }))
      .then((e) => {
        const [r, a] = g(e);
        const message = { ...r };
        message["id"] = s;
        t.postMessage(message, a), "RELEASE" === c && (t.removeEventListener("message", n), i(t));
      });
  }),
    t.start && t.start();
}
function i(e) {
  (function (e) {
    return "MessagePort" === e.constructor.name;
  })(e) && e.close();
}
function c(e, r) {
  return (function e(r, a = [], s = function () {}) {
    let o = false;
    const c = new Proxy(s, {
      get(t, s) {
        if ((u(o), s === n))
          return () =>
            h(r, { type: "RELEASE", path: a.map((e) => e.toString()) }, undefined).then(() => {
              i(r), (o = !0);
            });
        if ("then" === s) {
          if (0 === a.length) return { then: () => c };
          const e = h(r, { type: "GET", path: a.map((e) => e.toString()) }, undefined).then(E);
          return e.then.bind(e);
        }
        return e(r, [...a, s]);
      },
      set(e, t, n) {
        u(o);
        const [s, i] = g(n);
        return h(r, { type: "SET", path: [...a, t].map((e) => e.toString()), value: s }, i).then(E);
      },
      apply(n, s, i) {
        u(o);
        const c = a[a.length - 1];
        if (c === t) return h(r, { type: "ENDPOINT" }, undefined).then(E);
        if ("bind" === c) return e(r, a.slice(0, -1));
        const [p, m] = l(i);

        /** @noinline */
        const applyForm = {};

        applyForm["type"] = "APPLY";

        applyForm["path"] = a.map((e) => e.toString());

        applyForm["argumentList"] = p;

        return h(r, applyForm, m).then(E);
      },
      construct(e, t) {
        u(o);
        const [n, s] = l(t);
        /** @noinline */
        const constructForm = {};
        constructForm["type"] = "CONSTRUCT";
        constructForm["path"] = a.map((e) => e.toString());
        constructForm["argumentList"] = n;
        return h(r, constructForm, s).then(E);
      },
    });
    return c;
  })(e, [], r);
}
function u(e) {
  if (e) throw new Error("Proxy has been released and is not useable");
}
function l(e) {
  const t = e.map(g);
  return [t.map((e) => e[0]), ((n = t.map((e) => e[1])), Array.prototype.concat.apply([], n))];
}
const p = new WeakMap();

function m(e, t) {
  return p.set(e, t), e;
}

function d(t) {
  return Object.assign(t, { [e]: !0 });
}

function f(e, t = self, n = "*") {
  return {
    postMessage: (t, r) => e.postMessage(t, n, r),
    addEventListener: t.addEventListener.bind(t),
    removeEventListener: t.removeEventListener.bind(t),
  };
}
function g(e) {
  for (const [t, n] of s)
    if (n.canHandle(e)) {
      const [r, a] = n.serialize(e);
      return [{ type: "HANDLER", name: t, value: r }, a];
    }
  return [{ type: "RAW", value: e }, p.get(e) || []];
}
function E(e) {
  switch (e.type) {
    case "HANDLER":
      return s.get(e.name).deserialize(e.value);
    case "RAW":
      return e.value;
  }
}
function h(e, t, n) {
  return new Promise((r) => {
    const a = new Array(4)
      .fill(0)
      .map(() => Math.floor(Math.random() * Number.MAX_SAFE_INTEGER).toString(16))
      .join("-");
    e.addEventListener("message", function t(n) {
      n.data && n.data.id && n.data.id === a && (e.removeEventListener("message", t), r(n.data));
    });
    e.start && e.start();
    const message = t;
    message["id"] = a;
    console.log("message2", message);
    e.postMessage(message, n);
  });
}

export {
  t as createEndpoint,
  o as expose,
  d as proxy,
  e as proxyMarker,
  n as releaseProxy,
  m as transfer,
  s as transferHandlers,
  f as windowEndpoint,
  c as wrap,
};
