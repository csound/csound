<CsTest>
description = "a new-style UDO that has outputs must have an xout"

[expect]
exit = "nonzero"
stderr = ["UDO Missing has outputs k, a and no xout"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 4

; without the xout, the caller's variables keep what they held: 5 and 7
opcode Missing():(k, a)
  unused:k = 1
endop

instr 1
  x:k init 5
  y:a = 7
  x, y = Missing()
  printks("x %g y %g\n", 0, x, vaget(0, y))
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
