// A standalone uploaded WASI command: (memory (export "memory") 1),
// (import "wasi_snapshot_preview1" "proc_exit" (func (param i32))),
// (func (export "_start") (call 0 (i32.const status))).
export function exitCommand(status) {
  const name = (text) => [text.length, ...new TextEncoder().encode(text)];
  const section = (id, bytes) => [id, bytes.length, ...bytes];
  return new Uint8Array([
    0,
    97,
    115,
    109,
    1,
    0,
    0,
    0,
    ...section(1, [2, 0x60, 1, 0x7f, 0, 0x60, 0, 0]),
    ...section(2, [1, ...name("wasi_snapshot_preview1"), ...name("proc_exit"), 0, 0]),
    ...section(3, [1, 1]),
    ...section(5, [1, 0, 1]),
    ...section(7, [2, ...name("memory"), 2, 0, ...name("_start"), 0, 1]),
    ...section(10, [1, 6, 0, 0x41, status, 0x10, 0, 0x0b]),
  ]);
}

export const beatsCsd = (program = "csbeats") => `
<CsoundSynthesizer>
<CsOptions>
-oscore-test.wav -W -s
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  prints "generated frequency %.4f\\n", p4
  out poscil(0.2, p4)
endin
</CsInstruments>
<CsScore bin="${program}">
i1 m1 b1 C4 q mf
</CsScore>
</CsoundSynthesizer>
`;

// A second uploaded command swaps files through the real path_rename import.
export function swapCommand() {
  const name = (text) => [text.length, ...new TextEncoder().encode(text)];
  const section = (id, bytes) => [id, bytes.length, ...bytes];
  const rename = (from, fromLength, to, toLength) => [
    ...[3, from, fromLength, 3, to, toLength].flatMap((value) => [0x41, value]),
    0x10,
    0,
    0x1a,
  ];
  const body = [0, ...rename(0, 1, 4, 4), ...rename(2, 1, 0, 1), ...rename(4, 4, 2, 1), 0x0b];
  return new Uint8Array([
    0,
    97,
    115,
    109,
    1,
    0,
    0,
    0,
    ...section(1, [2, 0x60, 6, ...Array(6).fill(0x7f), 1, 0x7f, 0x60, 0, 0]),
    ...section(2, [1, ...name("wasi_snapshot_preview1"), ...name("path_rename"), 0, 0]),
    ...section(3, [1, 1]),
    ...section(5, [1, 0, 1]),
    ...section(7, [2, ...name("memory"), 2, 0, ...name("_start"), 0, 1]),
    ...section(10, [1, body.length, ...body]),
    ...section(11, [1, 0, 0x41, 0, 0x0b, ...name("a\0b\0temp")]),
  ]);
}
