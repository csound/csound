<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Test: instr definition missing id.
; Expected: parse failure without any crash.

instr
  nonsense()
endin

</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
