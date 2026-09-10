<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  iwindow = (p4 == 0 ? sqrt:i(-1) : p4)
  ain = 0
  kout downsamp ain, iwindow
endin
</CsInstruments>
<CsScore>
i 1 0 .001 0
i 1 0 .001 -1
i 1 0 .001 9
i 1 0 .001 1e30
</CsScore>
</CsoundSynthesizer>
