// A child instance initially points at the parent's stack. Plugin start/TLS
// code may use C stack frames before wasi_thread_start installs the pthread stack.
export function withWasiThreadStack(exports, stackTop, callback) {
  if (!stackTop) return callback();
  const stack = exports["__stack_pointer"];
  if (!(stack instanceof WebAssembly.Global))
    throw new TypeError("Threaded plugins require an exported stack pointer");
  const previous = stack.value;
  stack.value = stackTop;
  try {
    return callback();
  } finally {
    stack.value = previous;
  }
}
