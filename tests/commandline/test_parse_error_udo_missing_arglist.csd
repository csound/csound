<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Test: UDO signature missing argument list.
; Expected: parse failure without any crash.

opcode myop (a) :
  aout = 0
endop

</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
