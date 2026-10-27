<CsTest>
description = "A typed variable declared in an if body inside a UDO is not visible after endif"
[expect]
exit = "nonzero"
stderr = ["'r' is declared inside a block", "and is not visible outside it"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode f(n:i):i
  if n == 1 then
    r:i = 99
  endif
  xout(r)
endop

instr 1
  v:i = f(p4)
  prints("v=%d\n", v)
endin
</CsInstruments>
<CsScore>
i1 0 0.1 1
i1 0.2 0.1 0
</CsScore>
</CsoundSynthesizer>
