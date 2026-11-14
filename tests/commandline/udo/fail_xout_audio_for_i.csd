<CsTest>
description = "a new-style UDO's xout cannot give an a for an i output"

[expect]
exit = "nonzero"
stderr = ["xout in UDO AudioToI gives a for output 1, which is i"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
opcode AudioToI():i
  sig:a = 1
  xout sig
endop

instr 1
  value:i = AudioToI()
  print value
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
