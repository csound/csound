<CsoundSynthesizer>

<CsInstruments>
nchnls = 3
0dbfs = 1

instr 1
  a1 oscil 0.8, 440
  a2 oscil 0.6, 880
  az = 0
  alow line -1, p3, -1
  ahigh line 1, p3, 1
  aout  select a1,a2,alow,az,ahigh
  outch 1, a1, 2,a2, 3,aout
  endin
</CsInstruments>

<CsScore>
i1 0 5
e
</CsScore>

</CsoundSynthesizer>
