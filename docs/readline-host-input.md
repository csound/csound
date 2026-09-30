# Host input for readline

The Csound Web IDE needs to show a text field when an instrument asks for a
line. A check at startup cannot cover instruments scheduled later, repeated
lines, or instruments stopped while waiting. Terminal output also cannot tell
the IDE when an empty prompt requests input.

The host API reports each request as it opens and closes. It does not require
parsing the orchestra or console output.

## C hosts

Register `csoundSetReadlineCallback(csound, callback, userData)` before starting
performance. This selects host input instead of reading or writing the terminal.
The callback receives `(csound, userData, requestId, prompt)`:

- A non-NULL UTF-8 prompt opens a request. An empty string still requests input.
- A NULL prompt closes that request.
- Each line gets a new, nonzero ID. IDs belong to one Csound instance and are
  not reused across resets. The callback and user data also survive reset.

The callback runs on the thread executing Csound. Copy the prompt before
returning and send the copy to the UI without blocking the audio thread. Do not
call UI code or re-enter Csound in this callback, except to submit a line.
Register or replace callbacks only while performance is stopped, before any
readline instrument is active.

Submit an answer with `csoundReadlineSubmit(csound, requestId, text)` on the
performance thread, or while performance is suspended. Pass one complete line
without its newline. Empty text submits an empty line. The call appends the
newline and returns `CSOUND_SUCCESS` if accepted. It closes the request at once;
the opcode consumes the queued bytes on subsequent control periods.

The call rejects stale IDs, duplicate answers, control bytes other than tab,
and input that does not fit in the queue. Do not mix it with keyboard input or
`csoundReadlinePushText()` for the same line. The existing push-text API remains
available for hosts that deliberately queue input ahead of time.

Requests also close on EOF, an error, instrument deinitialization, or reset.
Stopping performance without running cleanup may leave an instrument allocated;
a native UI should also clear its field when the host stops performance.

## Browser hosts

The browser package installs the callback when it creates Csound. Attach a
listener before compiling/starting playback:

```js
let pending;
csound.on("readline", ({ requestId, prompt }) => {
  if (prompt === null) {
    if (pending === requestId) {
      pending = undefined;
      hideInput();
    }
  } else {
    pending = requestId;
    showInput(prompt); // Also show the field when prompt is "".
  }
});

async function submitLine(text) {
  const requestId = pending;
  if (requestId === undefined) return;
  const result = await csound.readlineSubmit(requestId, text);
  if (result !== 0) {
    // The request may have closed while the user was typing.
    showInputError("Csound could not accept this line.");
  }
}
```

The browser sends `readline` events to the main thread in all three audio modes:
single AudioWorklet, worker with shared memory, and worker with message ports.
The events stay separate from console messages. Stopping playback or ending a
render closes the visible request as well.

For synchronous use without WebAudio, pass `onReadline` to `libcsound()`.
It receives `{csound, requestId, prompt}`; the extra pointer identifies which
Csound instance requested the line. Submit with
`api.csoundReadlineSubmit(csound, requestId, text)`.

Both the WASM binary and browser wrapper must include this feature. Publish the
new `@csound/wasm-bin` build and update the browser package's binary dependency
before releasing it for the Web IDE. This change supplies the host API; the Web
IDE's text field will use it in a separate change.
