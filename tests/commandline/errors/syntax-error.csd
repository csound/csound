<CsTest>
description = "errors/syntax-error.csd"

[expect]
exit = "nonzero"
stderr = ["syntax error, unexpected ERROR_TOKEN"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-odac -m0
</CsOptions>
<CsInstruments>
; Intentionally malformed ORC to trigger lexer/parser messages
instr 1
  @@@ THIS IS INVALID SYNTAX @@@
endin
</CsInstruments>
<CsScore>
i1 0 0.1
</CsScore>
</CsoundSynthesizer>
