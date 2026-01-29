<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Test: opcode definition missing name.
; Expected: parse failure without any crash.

opcode , a, k
  aout = 0
endop

</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
