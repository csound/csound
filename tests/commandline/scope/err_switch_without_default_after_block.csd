<CsTest>
description = "A name declared in every case of a switch without default is not visible after endsw"
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
  switch p4
  case 1
    v:i = 1
  case 2
    v:i = 2
  endsw
  prints("v=%d\n", v)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
</CsScore>
</CsoundSynthesizer>
