<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

gishape ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1

instr 1
  kdist = sqrt(-1)
  asignal init 0
  aresult distort asignal, kdist, gishape
  out aresult
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
