<CsTest>
description = "array-to-PVS conversion rejects an incomplete amplitude/frequency pair"

[expect]
exit = "nonzero"
stderr = ["tab2pvs: expected a one-dimensional array of at least two amplitude/frequency pairs"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1

instr IncompletePair
  ; 65 values leave the final amplitude without a frequency.
  ; Reject the array here, before any other opcode receives the frame.
  kInput[] init 65
  fInput pvsfromarray kInput, 32
endin
</CsInstruments>
<CsScore>
i "IncompletePair" 0 .01
</CsScore>
</CsoundSynthesizer>
