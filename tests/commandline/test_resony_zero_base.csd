<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
  aInput = .01
  kBase linseg 1000, .004, 0, .004, 0
  aOutput resony aInput, kBase, 100, 3, 400, 1, 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
