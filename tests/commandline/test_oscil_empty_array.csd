<CsTest>
description = "Array oscillators reject an empty waveform"

[expect]
exit = "nonzero"
stderr = ["oscil: invalid waveform array size or dimensions"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
instr 1
  iWave[] init 0
  aSignal oscili 1, 440, iWave
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
