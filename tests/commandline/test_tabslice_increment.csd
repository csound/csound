<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
opcode assertK,0,kk
  kActual, kExpected xin
  if kActual != kExpected then
    printks "tabslice returned %f, expected %f\n", 1, kActual, kExpected
    exitnowk(-1)
  endif
endop

instr 1
  src:k[] init 4
  src fillarray 1, 2, 3, 4
  dst:k[] init 3
  dst tabslice src, 1, 3, 1
  assertK dst[0], 2
  assertK dst[1], 3
  assertK dst[2], 4
  turnoff
endin

instr 2
  src:k[] init 4
  src fillarray 1, 2, 3, 4
  dst:k[] tabslice src, 0, 3, 0
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
</CsScore>
</CsoundSynthesizer>
