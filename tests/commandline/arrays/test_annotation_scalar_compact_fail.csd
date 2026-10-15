<CsTest>
description = "a scalar UDO result cannot satisfy a compact array annotation"
args = ["-nd", "--syntax-check-only"]

[expect]
exit = "nonzero"
stderr = ["Unable to find opcode entry for 'Scalar'"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode Scalar(value:k):k
  xout value
endop

instr 1
  kValue init 1
  ; Expanding the input expression must not discard the array output type.
  arr:k[][] = Scalar(kValue + 1)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
