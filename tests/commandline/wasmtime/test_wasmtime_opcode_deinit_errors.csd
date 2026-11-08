<CsTest>
description = "Wasmtime opcode deinit errors"
skip = "Run by commandline_wasmtime_opcode_cache with its generated plugin"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --num-threads=1
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
  wasmdeiniterror p4
endin
instr 2
  iCalls wasmdeinitcount
  prints "WASM_DEINIT_ERRORS=%.0f\n", iCalls
endin
</CsInstruments>
<CsScore>
; The first callback returns NOTOK; the second calls an unbridged host API.
i 1 0 0.01 0
i 1 0.02 0.01 1
i 2 0.04 0.001
</CsScore>
</CsoundSynthesizer>
