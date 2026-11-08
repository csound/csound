<CsTest>
description = "hrtfreverb creates its FFT setups before using them during init"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
  asig oscili 0.5, 440
  arevl, arevr, idel hrtfreverb asig, 0.5, 0.5, "hrtf-44100-left.dat", "hrtf-44100-right.dat", 44100, 0.05
  outs arevl, arevr
endin
</CsInstruments>
<CsScore>
i 1 0 0.25
</CsScore>
</CsoundSynthesizer>
