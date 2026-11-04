<CsTest>
description = "A name declared with different types in the branches of an if is not visible after endif"
[expect]
exit = "nonzero"
stderr = ["'v' is declared inside a block", "and is not visible outside it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  if p4 == 1 then
    v:i = 1
  else
    v:S = "two"
  endif
  prints("v=%d\n", v)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
</CsScore>
</CsoundSynthesizer>
