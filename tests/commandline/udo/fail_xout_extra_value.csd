<CsTest>
description = "a new-style UDO's xout cannot give more values than it has outputs"

[expect]
exit = "nonzero"
stderr = ["xout in UDO Extra gives 2 values for 1 outputs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode Extra():k
  xout 1, 2
endop

instr 1
  value:k = Extra()
  printks("%g\\n", 0, value)
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
