<CsTest>
description = "A variable introduced by a for loop is not visible after the loop"
[expect]
exit = "nonzero"
stderr = ["'k' is declared inside a block", "and is not visible outside it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  for k in [1, 2, 3] do
    prints("%d\n", k)
  od
  prints("k=%d\n", k)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
i1 0.2 0.1 0
</CsScore>
</CsoundSynthesizer>
