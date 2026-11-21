<CsTest>
description = "a new-style UDO's xout cannot give a number for a string"

[expect]
exit = "nonzero"
stderr = ["xout in UDO Name gives k for output 2, which is S"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode Name():(k, S)
  value:k = 1
  xout value, value
endop

instr 1
  number:k, name:S = Name()
  printks("%g %s\\n", 0, number, name)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
