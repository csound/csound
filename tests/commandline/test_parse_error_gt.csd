<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Test: binary '>' missing RHS.
; Expected: parse failure without any crash.

instr 1
  ival init 0
  ival = 1 >
endin

</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
