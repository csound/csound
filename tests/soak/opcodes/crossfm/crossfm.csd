<CsoundSynthesizer>
<CsInstruments>
sr        =         96000
ksmps     =         10
nchnls    =         2
0dbfs     =         1


instr 1
  kamp linen 0.5, 0.01, p3, 0.5
  kfrq1 line 0, p3, sr/2
  kfrq2 line 0, p3, sr/2
  kndx1 line 0, p3, sr/2
  kndx2 line 0, p3, sr/2
  a1,a2 crossfm kfrq1, kfrq2, kndx1, kndx2, 1, 1, 1
  outs a1*kamp, a2*kamp
endin
</CsInstruments>
<CsScore>
f1 0 16384 10 1 0
i1 0 5
e
</CsScore>
</CsoundSynthesizer>
