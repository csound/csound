<CsTest>
description = "xout must match the rank declared in a UDO result"

[expect]
exit = "nonzero"
stderr = ["rank mismatch", "expected 1, got 2"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode WrongRank():i[]
  matrix:i[][] init 2, 3
  xout matrix
endop

instr 1
  vector:i[] = WrongRank()
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
