<CsTest>
description = "a new-style UDO's xout cannot give a string for a number"

[expect]
exit = "nonzero"
stderr = ["xout in UDO Text gives S for output 1, which is k"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode Text():k
  xout "hello"
endop

instr 1
  value:k = Text()
  printks("%g\\n", 0, value)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
