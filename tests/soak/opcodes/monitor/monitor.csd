<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1
  asig poscil3 .5, 880, giSine
  outs asig, asig
endin

instr 2
  klfo lfo 1, 2, 0
  asig poscil3 .5*klfo, 220, giSine
  outs asig, asig
endin

instr 99 ;read the stereo csound output buffer
  allL, allR monitor
endin

</CsInstruments>
<CsScore>

i 1 0 2
i 2 0 3
i 99 0 3
e
</CsScore>
</CsoundSynthesizer>
