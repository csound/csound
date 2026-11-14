<CsTest>
description = "a new-style UDO's xout cannot give fewer values than it has outputs"

[expect]
exit = "nonzero"
stderr = ["xout in UDO Missing gives 1 values for 2 outputs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode Missing():(k, k)
  xout 1
endop

instr 1
  first:k, second:k = Missing()
  printks("%g %g\\n", 0, first, second)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
