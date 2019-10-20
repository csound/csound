<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

giSine ftgen 0, 0, 2^10, 10, 1

instr 1

kfreq     randh     1000, 20, 0.2, 1, 2000 ;generates ten random number between 100 and 300 per second
kpan      randh     .5, 1, 0.2, 1, .5   ;panning between 0 and 1
kp        lineto    kpan, .5          ;smoothing pan transition
aout      poscil    .4, kfreq, giSine
aL, aR    pan2      aout, kp
          outs      aL, aR

endin
</CsInstruments>
<CsScore>

i 1 0 10
e
</CsScore>
</CsoundSynthesizer>
