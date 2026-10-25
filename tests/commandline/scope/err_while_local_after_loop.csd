<CsTest>
description = "A typed variable declared in a while body is not visible after the loop"
[expect]
exit = "nonzero"
stderr = ["'t' is declared inside a block", "and is not visible outside it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  i:i init 0
  while i < p4 do
    t:i = 42
    i += 1
  od
  prints("t=%d\n", t)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
i1 0.2 0.1 0
</CsScore>
</CsoundSynthesizer>
