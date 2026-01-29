<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Test: UDO signature missing in-arg list after comma.
; Expected: parse failure without any crash.

opcode myop, a,
  aout = 0
endop

</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
