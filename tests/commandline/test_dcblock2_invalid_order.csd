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
  iorder = (p4 == 0 ? sqrt:i(-1) : p4)
  ain = 0
  aout dcblock2 ain, iorder
  out aout
endin
</CsInstruments>
<CsScore>
i 1 0 .001 0
i 1 0 .001 1e30
i 1 0 .001 -1e30
</CsScore>
</CsoundSynthesizer>
