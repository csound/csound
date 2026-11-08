<CsTest>
description = "expected failure: instr missing id"

[expect]
exit = "nonzero"
stderr = ["syntax error, unexpected T_IDENTB", "syntax error, unexpected ENDIN_TOKEN, expecting end of file"]
</CsTest>
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
