<CsTest>
description = "a new-style UDO's xout cannot give an array of another type"

[expect]
exit = "nonzero"
stderr = ["xout in UDO ControlToAudio gives k[] for output 1, which is a[]"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode ControlToAudio():a[]
  values:k[] = fillarray(1, 2)
  xout values
endop

instr 1
  values:a[] = ControlToAudio()
  printks("%d\\n", 0, lenarray(values))
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
