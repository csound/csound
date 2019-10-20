<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1
  kMoveUp  line 0, p3, sr/2
  outvalue "freq", kMoveUp
endin

instr 2
  kfreq invalue "freq"
  asig  oscil 0.1, kfreq, 1
  outs asig, asig
endin

</CsInstruments>
<CsScore>
f 1 0 1024 10 1 ;sine
i 1 0 5
i 2 0 5
e
</CsScore>
</CsoundSynthesizer>
