<CsoundSynthesizer>
<CsOptions>
  -d -n --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

instr 1
  aSource oscils 0.2, 220, 0
  aFiltered velvetlp aSource, 1200
  out aFiltered
endin
</CsInstruments>
<CsScore>
i 1 0.0001 0.019
e
</CsScore>
</CsoundSynthesizer>
