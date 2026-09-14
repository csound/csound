<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  aout granule 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0.002, 0, 50, 50, 0.5, 1, 1, 1, 1, 0
  out aout
endin
</CsInstruments>
<CsScore>
f 1 0 1024 7 1 1024 1
i 1 0 0.001
</CsScore>
</CsoundSynthesizer>
