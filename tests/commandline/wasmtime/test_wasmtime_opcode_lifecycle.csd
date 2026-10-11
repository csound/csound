<CsTest>
description = "Wasmtime opcode callback lifecycle"
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
; Reinit must keep the same opcode state.
instr 1
  kTime timeinstk
  if kTime == 2 then
    reinit again
  endif
again:
  iCalls wasmstate
  prints "WASM_REINIT=%.0f\n", iCalls
  rireturn
endin
; Tied notes must also keep their opcode state.
instr 2
  iCalls wasmstate
  prints "WASM_TIE=%.0f\n", iCalls
endin
; Each completed instrument must call deinit just once.
instr 3
  iCalls wasmdeinitcount
  prints "WASM_DEINIT_COUNT=%.0f\n", iCalls
endin
instr 4
  kOut init 17
  kOut wasmnoinit
  prints "WASM_NOINIT=%.0f\n", i(kOut)
  if kOut != 18 then
    printks "Wasm callback did not read the current output value: %.0f\n", 0, kOut
    exitnowk 1
  endif
  ; Each perf callback must read this value, not its previous guest output.
  kOut = 17
  iOut = 17
  iOut wasminitadd
  prints "WASM_INIT_ADD=%.0f\n", iOut
endin
; Writes must reach an input that aliases the output, but not a distinct input.
instr 5
  kOut init 17
  kOut wasmalias kOut
  kInput init 17
  kSeparate wasmalias kInput
  if kOut != 1 || kSeparate != 0 then
    printks "Wasm argument aliases differ: %.0f, %.0f\n", 0, kOut, kSeparate
    exitnowk 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 -0.01
i 2 0.03 0.01
i 3 0.06 0.001
i 4 0.07 0.01
i 5 0.09 0.01
</CsScore>
</CsoundSynthesizer>
