<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

instr 1

asig oscili p4, p5
a1,a2 hilbert2 asig,1024,256
am,afm fmanal a1,a2
ktrig metro 2
printf "AM=%.3f FM=%.1f\n",ktrig,k(am),k(afm)
  outs a1, a2

endin


</CsInstruments>
<CsScore>
i1 0 10 0.5 440
</CsScore>
</CsoundSynthesizer>
