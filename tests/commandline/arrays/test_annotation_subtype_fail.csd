<CsTest>
description = "an array annotation cannot silently change the element type"

[expect]
exit = "nonzero"
stderr = ["values:S[] -- type mismatch for existing array variable values:i[]"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  values:i[] init 2
  ; This must fail just as it does with a space before the brackets.
  values:S[] init 2
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
