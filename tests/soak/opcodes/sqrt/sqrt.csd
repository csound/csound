<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
0dbfs  = 1
nchnls = 2

instr 1
  asig   pluck 0.7, 55, 55, 0, 1
  kpan   line 0,p3,1
  kleft  = sqrt(1-kpan)
  kright = sqrt(kpan)
  outs asig*kleft, asig*kright
endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
