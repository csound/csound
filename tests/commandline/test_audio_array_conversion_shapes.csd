<CsTest>
description = "Audio block conversions reject matrices instead of treating the first dimension as the sample count"

[expect]
exit = "nonzero"
stderr = ["a(k[]): expected an initialized one-dimensional array", "Audio-to-array conversion requires a one-dimensional output"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

instr MatrixInput
  ; A 2-by-4 matrix contains eight numbers, not two audio samples.
  kMatrix[][] init 2, 4
  aResult = a(kMatrix)
endin

instr MatrixOutput
  aInput = 1
  kMatrix[][] init 2, 4
  kMatrix = array(aInput)
endin
</CsInstruments>
<CsScore>
i "MatrixInput" 0 .25
i "MatrixOutput" .5 .25
e
</CsScore>
</CsoundSynthesizer>
