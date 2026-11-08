<CsTest>
description = "slicearray rejects output buffer growth under --realtime"
args = []
[expect]
exit = "nonzero"
stderr = ["opcode slicearray", "Array too small", "cannot allocate during performance pass"]
output_excludes = ["warning: realloc", "AddressSanitizer", "runtime error:"]
[profiles.wasm]
skip = "The command-line WASI runtime does not support --realtime."
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -+rtaudio=null -d -m4 --realtime
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  ; Reserve three source elements but initially expose only one.
  aSource[] init 3
  trimi aSource, 1
  aSlice[] slicearray aSource, 0

  ; The source has enough capacity for this change. On the next block,
  ; slicearray needs a larger output buffer and must report a perf error.
  trim aSource, 3
endin
</CsInstruments>
<CsScore>
i 1 0 .25
e
</CsScore>
</CsoundSynthesizer>
