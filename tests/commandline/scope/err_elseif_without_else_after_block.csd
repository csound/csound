<CsTest>
description = "A name declared in every branch of an if without else is not visible after endif"
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
  elseif p4 == 2 then
    v:i = 2
  endif
  prints("v=%d\n", v)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
i1 0.2 0.1 0
</CsScore>
</CsoundSynthesizer>
