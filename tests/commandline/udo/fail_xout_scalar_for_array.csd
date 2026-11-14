<CsTest>
description = "a new-style UDO's xout cannot give a number for an array"

[expect]
exit = "nonzero"
stderr = ["xout in UDO NumberToArray gives c for output 1, which is k[]"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode NumberToArray():k[]
  xout 1
endop

instr 1
  values:k[] = NumberToArray()
  printks("%d\\n", 0, lenarray(values))
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
