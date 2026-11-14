<CsTest>
description = "a new-style UDO's xout cannot give a B for a b output"

[expect]
exit = "nonzero"
stderr = ["xout in UDO BFromB gives B for output 1, which is b"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode BFromB(value:k):b
  held:B = value > 0
  xout held
endop

instr 1
  flag:b = BFromB(1)
  print flag
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
