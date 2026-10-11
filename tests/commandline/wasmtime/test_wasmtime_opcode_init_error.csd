<CsTest>
description = "Wasmtime opcode cleanup after failed init"
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
  ; This init changes opcode state before returning NOTOK.
  iOut wasminiterror
endin
instr 2
  ; Deinit must see that state and run once, despite the init error.
  iCalls wasmdeinitcount
  prints "WASM_FAILED_INIT_CLEANUP=%.0f\n", iCalls
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 0.001
</CsScore>
</CsoundSynthesizer>
