<CsTest>
description = "expected failure: udo missing commas"

[expect]
exit = "nonzero"
stderr = ["syntax error, unexpected UDO_IDENT, expecting ',' or '('"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

; Test: UDO signature missing commas.
; Expected: parse failure without any crash.

opcode myop a k
  aout = 0
endop

</CsInstruments>
<CsScore>
e 0.1
</CsScore>
</CsoundSynthesizer>
