# Real-time mode architecture

Real-time performance must finish each audio block before its deadline. Keep
heap allocation and reallocation out of performance callbacks: an allocator
can block or move memory, even when a request looks small. Reserve storage
during initialization and reuse it during performance.

## Mode selection

`--realtime` selects Csound's real-time mode. Engine code reads
`csound->oparms->realtime`; plugins can read the flag through `GetOParms()`.
Hosts can set `--realtime` with `csoundSetOption()` before starting Csound.
Check this mode flag when guarding a compatibility fallback. Audio-device
output such as `-odac` alone does not select it, and a host can use real-time
mode without opening a Csound audio device.

## Array capacity

An array's logical length and reserved capacity are separate. Performance
code may change the length within the existing capacity. If the requested
result needs more space, report a performance error before copying data or
publishing the larger length. Real-time code must not fall back to `ReAlloc`,
`tabinit`, or another helper that can allocate storage. Cloning shared storage
has the same restriction.

The helpers in [arrays.h](../include/arrays.h) make this distinction:

- `tabinit` and `ArrayEnsureCapacity` can allocate storage.
- `tabcheck` checks capacity and write access without allowing allocation.
  It reports an error when either check fails.

Allocating on another thread needs a separate design for ownership, lifetime,
and when the new storage becomes available. The performance callback must not
wait for that allocation.

## Compatibility outside real-time mode

Keep any exception local to the opcode that needs it. `slicearray` preserves
its earlier growth behavior only when `--realtime` is off. It warns on the
first growth after initialization. With `--realtime`, the same request fails
if the output lacks capacity. This exception does not permit other opcodes to
grow arrays during performance.

Users can reserve output space for their largest expected slice:

```csound
; giLast will range from 0 to 2, so reserve three elements.
aSlice[] init 3
aSlice slicearray gaClear, 0, giLast
```

The slice can then change length without growing its buffer. Choose a realistic
maximum: reserving an entire large source for every small slice can waste
memory across many voices.

`slicearray` rereads its i-rate bounds during performance; it does not freeze
them at initialization. Another note's init pass can change a directly supplied
global such as `giLast`. Document this behavior and the capacity requirement
when explaining the opcode.

When changing these paths, check both modes. A real-time growth request must
fail before reaching an allocating helper; a preallocated output must still
work. Cover any supported non-real-time fallback separately.
