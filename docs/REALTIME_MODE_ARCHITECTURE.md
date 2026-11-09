# Real-time mode architecture

Real-time performance must finish each audio block before its deadline. Keep
work on the performance thread bounded, and move work that can wait to setup,
an appropriate worker, or shutdown. This applies to host callbacks and opcode
performance routines as well as the engine.

This document describes the design rules and the limits of the current
implementation. It does not claim that every opcode or API call is safe for
an audio callback. Read the contract for each call in
[csound.h](../include/csound.h), and check the path that executes it.

Three properties need separate checks:

- **Thread safety:** may this call overlap another call or performance?
- **Real-time safety:** can it finish without allocation, blocking I/O,
  waiting for another thread, or an unbounded amount of work?
- **Completion and lifetime:** has the work run, and who still uses its data?

A mutex can make a call thread-safe while making it unsuitable for audio
processing. An async call can return before completion while still allocating
on its caller. A pointer can remain valid while its contents change elsewhere.

## Mode selection

`--realtime` selects Csound's real-time mode. Engine code reads
`csound->oparms->realtime`. Plugins can read the flag through `GetOParms()`.
Hosts can set `--realtime` with `csoundSetOption()` before starting Csound.
Check this mode flag when guarding a compatibility fallback. Audio-device
output such as `-odac` alone does not select it, and a host can use real-time
mode without opening a Csound audio device.

Real-time event delivery is also distinct from `--realtime`. For example,
calling `csoundStart()` before compiling selects the scoreless performance
flow described in `csound.h`. It does not by itself select the background init
mode. Likewise, the `async` argument on an API call selects that call's queue
path, not the engine mode.

Check startup results and platform support. The background init path needs
working threads, atomics, and the required lock backend. Some builds reject
that path or warn and leave `--realtime` disabled. The checks live in
[argdecode.c](../Top/argdecode.c), [main.c](../Top/main.c), and
[musmon.c](../Engine/musmon.c). A host still has audio deadlines when the flag
is off.

## Performance ownership and lifetime

Use one host performance thread per Csound instance. Do not overlap calls to
`csoundPerformKsmps()` or re-enter performance from a Csound callback. The
active-render check rejects nested rendering. It does not serialize two
callers. Internal multicore rendering does not change this host contract.
See [csound_perf.c](../Top/csound_perf.c) and
[PR #3434](https://github.com/csound/csound/pull/3434).

The host owns the order of setup, rendering, and teardown:

1. Create the instance, set options and callbacks, and prepare storage before
   performance. Follow the chosen compile/start sequence in `csound.h`.
2. During performance, use only calls whose contracts allow that overlap.
   Write the `csoundGetSpin()` buffer before a block and read
   `csoundGetSpout()` after it. These are engine working buffers, not a queue
   for another thread.
3. Stop host submissions and rendering before reset or destruction. If using
   `CsoundPerformanceThread`, stop and join it before destroying its Csound
   instance. Keep callback functions and their user data alive through cleanup.
4. After reset, obtain engine-owned buffers and channel/table pointers again.
   Callback reset rules vary: keyboard and readline callbacks, for example,
   survive reset.

Unlinking an instrument or replacing a callback does not prove that no thread
still uses it. Engine retirement must account for active init work and readers
in the current render interval before freeing or reusing storage. See the
instance state and retirement comments in [insert.c](../Engine/insert.c).

## Locks and work on the performance thread

Do not fix an API race by adding a blocking lock to the performance path.
Prefer a clear caller restriction, preparation before performance, or a queue
with explicit ownership and failure behavior. A low-priority thread can hold
a lock while the audio thread needs it, causing a missed deadline.

`csoundPerformKsmps()` takes `API_lock` outside `--realtime` mode and omits it
in that mode. The presence of `API_lock` elsewhere therefore does not make an
arbitrary call safe during realtime rendering. The table-copy helpers in
[threadsafe.c](../Top/threadsafe.c) explicitly forbid taking the init mutex
on the audio side, even with a try-lock: the Bela path cannot use it there.
See [PR #3432](https://github.com/csound/csound/pull/3432).

Existing channel, allocation, and instance locks are not permission to add
more waits. Nor does a name such as `csoundSpinLock` prove real-time safety:
[threads.c](../Top/threads.c) uses different primitives on different systems,
including `os_unfair_lock` on modern macOS. Check the actual backend, all lock
holders, and how contention ends. Keep user callbacks and slow destruction
outside internal lock sections.

The asynchronous `diskin2` path shows the intended division: performance posts
a stop flag. Workers handle registry changes and deferred closes. Do not move
file close, worker joins, or registry locks back to the audio side. See
[diskin2.c](../OOps/diskin2.c) and
[PR #3238](https://github.com/csound/csound/pull/3238). The earlier
[PR #2609](https://github.com/csound/csound/pull/2609) records how joining a
worker while holding the allocation lock caused a deadlock.

## Queues and execution threads

These queues have different consumers and completion rules:

| Path | Where the work runs | Limit to keep in mind |
| --- | --- | --- |
| Async engine API requests | `message_dequeue()` during a control period | Submission may allocate. Acceptance does not mean completion. |
| Realtime allocation/init queue | `event_insert_thread()` | Init, reinit, and compiled-state merge can finish after later audio blocks. |
| Realtime message output | The init worker drains formatted messages | Output can be dropped. It must not make the audio producer wait. |
| `CsoundPerformanceThread` commands | The wrapper's performance thread, before rendering a block | This separate queue uses locks and can execute expensive host requests. |

### Engine API queue

[threadsafe.c](../Top/threadsafe.c) reserves slots and publishes each complete
payload with a readiness flag. Submission bounds both capacity and reservation
attempts. The audio consumer snapshots the end of the queue and stops at an
unpublished head. It never waits for a delayed producer. Requests added during
the drain wait for a later control period. Preserve those limits when changing
the queue. Draining until producers stop can starve audio.

The submitting thread allocates and copies payloads and reclaims old slot
payloads. Thus `async = 1` is not a promise that submission is safe in a strict
audio callback. Concurrent async submission needs the atomic backend. Without
it, submissions and performance must not overlap.

For `csoundEvent()`, `csoundEventString()`, and the async table-copy calls,
check `csoundGetAsyncStatus()` if rejection matters. A full or unavailable
queue can reject work, and the warning occurs at most once per instance
between resets. Success means **queued**, not run. The status belongs to the
instance, not to a request or thread. Csound's UDP input and host callbacks
can also change it. A lock around host calls cannot coordinate Csound's UDP
producer. Hosts with several submitting threads must coordinate a submission
and its status check. A rejection can call the message handler inside that
submission, so a host lock used there must allow re-entry. Such a
lock belongs off the audio path. See `csound.h` and
[PR #3524](https://github.com/csound/csound/pull/3524).

`csoundEvent()` does nothing before `csoundStart()`. Before start, use
`csoundEventString(..., async = 0)` to add score text. After start it schedules
realtime events. Async `csoundCompileCSD()` can also set the status when it
queues score text, but leaves it unchanged if it queues none. Read the return
value and the score-submission status for their separate purposes.

Payload ownership differs by call:

| Async call | Data the queue keeps | Caller obligation |
| --- | --- | --- |
| `csoundEvent()` | A copy of the p-fields | Check acceptance. Do not infer that the note has started. |
| `csoundEventString()` | A copy of the string | Check acceptance. Parsing and scheduling happen later. |
| `csoundTableCopyIn()` | The source address | Keep the full table plus guard point valid and unchanged until the copy runs. |
| `csoundTableCopyOut()` | The destination address | Reserve the full table length, excluding the guard point, and do not access it concurrently with the copy. |

For table copies, also keep the engine table storage valid and prevent
concurrent table changes until the copy completes. A missing table when the
request runs produces no copy. The async status is not a buffer-release signal.
Do not use it to free or reuse a borrowed buffer. Synchronous table copies in
`--realtime` mode take no mutex and must not overlap rendering, table changes,
or other host table copies. `csoundGetTable()` and `csoundGetTableArgs()`, and
the pointers they return, are not thread-safe.

Queueing safely and scheduling at the intended musical time are separate
problems. Use the event API's time rules and the engine's clock. Do not assume
that a host thread submits at an exact block boundary. This distinction also
appears in the older [issue #772](https://github.com/csound/csound/issues/772).
Use the current headers for API names and contracts.

### Background init and compilation

In `--realtime` mode, the init worker handles queued instrument allocation,
init/reinit work, and merges of newly compiled state. Rendering checks instance
readiness instead of waiting for init to finish. Preserve readiness and
turnoff state together: an instance that expires during init must not later
become runnable. See [insert.c](../Engine/insert.c) and
[PR #3434](https://github.com/csound/csound/pull/3434).

`csoundCompileOrc(..., async = 1)` still parses and compiles on the caller.
When adding code to a running engine, it queues the merge. In `--realtime`
mode the init worker performs that merge and its global init work. Outside
that mode, the engine API queue performs the merge. Check the call's return
value for queue rejection. Do not move compilation into an audio callback
because the argument says `async`. See
[csound_orc_compile.c](../Engine/csound_orc_compile.c) and
[PR #2590](https://github.com/csound/csound/pull/2590).

The init worker takes `init_pass_threadlock` before `alloc_spinlock` where
both are needed. It releases the allocation lock around user init code,
which may acquire that lock itself. Preserve this order and the scope of
each lock. On shutdown, close admission before joining the worker, drain
accepted init work, then free its queues. These are internal worker rules,
not permission to wait from a performance callback. See
[PR #3430](https://github.com/csound/csound/pull/3430) for queue shutdown.

### Message output

The realtime log path in [insert.c](../Engine/insert.c) copies formatted text
into a fixed queue. A producer makes one attempt to claim it and drops output
on contention or lack of space. The worker calls the saved host output
handler without holding the queue lock and bounds each drain so a callback
that logs again cannot keep it there forever. Shutdown joins the worker before
restoring callbacks and freeing the queue. Late output must not bypass that
queue and call host logging on the audio producer.

Register output handlers before starting. Keep them alive through shutdown,
and do not assume messages always arrive on the UI thread or only one thread.
Copy text that must outlive the callback. Logging is not a reliable command
or completion channel: realtime output can be truncated or dropped. The
current behavior has regression coverage in
[realtime_logging_test.cpp](../tests/c/realtime_logging_test.cpp).

### Performance-thread wrapper

`CsoundPerformanceThread` has its own command queue, separate from the engine
API and init queues. Its `CompileOrc()` command calls synchronous compilation
on the performance thread. `EvalCode()` and requested callbacks run there too.
Moving a call into this queue prevents some host-thread races but does not
remove its cost from the audio schedule. The process callback runs before each
rendered block. See [csPerfThread.cpp](../Top/csPerfThread.cpp).

While performance continues, `FlushMessageQueue()` waits until the wrapper
queue is empty and no queued command or its callback is in flight. Exit can
discard pending commands, so a completed flush does not prove they all ran.
It is not a barrier for engine async requests, background init work, the
process callback, or the next audio block. Never flush or join from the
performance thread. The flush implementation warns and ignores a call from
that thread to avoid deadlock. `Stop()` queues a stop. `Join()` waits for exit.
Follow the contracts in
[csPerfThread.hpp](../include/csPerfThread.hpp).

## Host callbacks

Each callback needs an execution-thread, re-entry, and lifetime contract.
Keeping a function pointer alive does not make callback registration or
removal thread-safe.

- Keyboard callbacks run from input-consuming opcodes during performance.
  The current list walker in [csound.c](../Top/csound.c) does not support
  concurrent mutation or mutation from inside a keyboard callback. Register,
  re-register, or remove callbacks while performance is stopped, or at a host
  boundary where no callback walk can run. Self-removal can free the very entry
  the walker will next read. The discussion in
  [PR #3631](https://github.com/csound/csound/pull/3631#issuecomment-6739759454)
  rejects adding a performance-thread lock. It does not establish a new
  thread-safe removal API.
- Readline callbacks can run on the audio thread. Do not block, call UI code,
  or re-enter Csound from them except through `csoundReadlineSubmit()`. The
  prompt is valid only during the callback. Queue any UI work in the host.
  Serialize the setter and submissions with performance/reset as `csound.h`
  requires.
- Keep function pointers, language-binding callback objects, and user data
  alive for every possible call, including one already in flight. Replacing a
  callback does not wait for such a call to return. For example, the
  performance-thread wrapper snapshots its process callback under a lock and
  invokes it after releasing the lock. The binding fixes in
  [PR #3594](https://github.com/csound/csound/pull/3594) and
  [PR #3613](https://github.com/csound/csound/pull/3613) preserve callback
  objects. They do not make arbitrary host data access safe.

## Channels and shared data

Channel APIs have their own synchronization. They do not rely on `API_lock`
or the engine API queue. Prefer the typed channel APIs when sharing values,
but check their allocation and locking costs before calling them from audio.
`csoundGetChannelPtr()` can create a channel. Set up channels and reserve their
storage before performance where possible.

Direct access through a channel pointer is not inherently thread-safe.
Follow `csound.h`: use the supported atomic operations for control values,
and channel locks for audio/string data. Keep locked copies short, with no
allocation, I/O, or callbacks while holding the lock. Audio buffers need room
for `ksmps` values. Do not wrap a getter that takes the channel lock inside
another acquisition of that lock.

String sizing needs special care. `csoundGetChannelDatasize()` followed by
`csoundGetStringChannel()` is unsafe if another writer can grow the string
between calls: the getter has no destination-size argument. The header gives
a bounded-copy example using a preallocated buffer under the channel lock.
The lock does not protect orchestra assignments to a variable exported with
`chnexport`. See [PR #3586](https://github.com/csound/csound/pull/3586).

Managed values need more than a byte-copy rule. `csoundSetChannel()` can run a
type's allocating `copyValue` while holding the destination lock. Its source
needs separate protection if it is live engine data. `csoundGetChannel()`
returns engine-owned storage whose changing contents still need protection.
String setters can also reallocate. Do not treat these calls as safe for the
performance thread merely because they serialize a write.

The channel binding cache in [bus.c](../OOps/bus.c) avoids repeated registry
lookups for unchanged opcode channel names. A name change can still need a
locked lookup, and value locks remain. Preserve that distinction when
extending the cache. See [PR #3508](https://github.com/csound/csound/pull/3508).

## Allocation and array capacity

Keep heap allocation, reallocation, and freeing out of new performance paths:
an allocator can block or move memory, even when a request looks small.
Reserve storage during initialization and reuse it during performance.

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

### Compatibility outside real-time mode

Keep any exception local to the opcode that needs it. `slicearray` preserves
its earlier growth behavior only when `--realtime` is off. It warns on the
first growth after initialization. With `--realtime`, the same request fails
if the output lacks capacity. This exception does not permit other opcodes to
grow arrays during performance. See the compatibility report in
[issue #3551](https://github.com/csound/csound/issues/3551) and its fix in
[PR #3552](https://github.com/csound/csound/pull/3552).

Users can reserve output space for their largest expected slice:

```csound
; giLast will range from 0 to 2, so reserve three elements.
aSlice[] init 3
aSlice slicearray gaClear, 0, giLast
```

The slice can then change length without growing its buffer. Choose a realistic
maximum: reserving an entire large source for every small slice can waste
memory across many voices.

`slicearray` rereads its i-rate bounds during performance. It does not freeze
them at initialization. Another note's init pass can change a directly supplied
global such as `giLast`. Document this behavior and the capacity requirement
when explaining the opcode.

When changing these paths, check both modes. A real-time growth request must
fail before reaching an allocating helper. A preallocated output must still
work. Cover any supported non-real-time fallback separately.

## Reviewing a change

Trace the callers as well as the function being changed. Record which thread
runs each path, which data it owns, and when another thread can use or release
that data. Check normal operation, full queues, delayed init, callback
re-entry, errors, turnoff, reset, and shutdown. A successful uncontended run
does not check the failure path that could block audio.

Follow the [API and ABI compatibility rules](ABI_API_COMPATIBILITY.md) when
adding engine state. Existing private `CSOUND` fields must keep their layout.
Append new private fields only at the end of the entire private data section.
Do not place new queue, lock, or callback fields beside related existing
members if that shifts their offsets. Add plugin function-table entries
through reserved slots so the private section keeps its starting offset.

Keep API comments and this document aligned when changing a contract. Useful
regressions to read or extend include
[realtime_queue_test.cpp](../tests/c/realtime_queue_test.cpp),
[realtime_readiness_test.cpp](../tests/c/realtime_readiness_test.cpp),
[realtime_chain_test.cpp](../tests/c/realtime_chain_test.cpp),
[perfthread_test.cpp](../tests/c/perfthread_test.cpp),
[channel_binding_test.cpp](../tests/c/channel_binding_test.cpp), and
[slicearray_test.cpp](../tests/c/slicearray_test.cpp).
