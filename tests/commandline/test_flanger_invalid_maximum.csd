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
  asignal = .25
  adelay = 0
  imax = p4 == 0 ? strtod("nan") : p4
  aout flanger asignal, adelay, 0, imax
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 1e30
</CsScore>
</CsoundSynthesizer>
