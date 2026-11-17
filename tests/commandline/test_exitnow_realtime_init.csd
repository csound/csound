<CsTest>
description = "exitnow at init under --realtime ends the performance with its value"
args = ["-nd", "--realtime"]
profiles.wasip2.skip = "Requires a realtime init thread; WASI Preview 2 has no thread spawning"

[expect]
exit = 3
stderr = ["Initialising realtime allocation queue"]

# WASI ends the process on a jump from a worker thread, the init thread
# included: csoundLongJmp calls _Exit(EXIT_FAILURE) there (#3438).
[profiles.wasm.expect]
exit = 1
stderr = ["Initialising realtime allocation queue"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1

; The init thread runs this. Its jump used to land in the frames of the
; thread that performs, and the csound command died of a segmentation fault.
instr 1
  exitnow 3
endin
</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
