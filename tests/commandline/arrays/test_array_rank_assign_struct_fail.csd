<CsTest>
description = "struct-array assignment preserves rank"

[expect]
exit = "nonzero"
stderr = ["rank mismatch", "expected 1, got 2"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
struct Item value:i

instr 1
  source:Item[][] init 2, 3
  target:Item[] = source
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
