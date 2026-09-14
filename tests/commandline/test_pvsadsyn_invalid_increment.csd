<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  ain oscili 0.1, 440
  fsig pvsanal ain, 128, 32, 128, 1
  ioffset = (p5 == -2 ? sqrt:i(-1) : p5)
  iincr = (p4 == -2 ? sqrt:i(-1) : p4)
  aout pvsadsyn fsig, 2, 1, ioffset, iincr
  out aout
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 0
i 1 0 .01 2147483647 0
i 1 0 .01 -1 0
i 1 0 .01 -2 0
i 1 0 .01 1 -1
i 1 0 .01 1 -2
i 1 0 .01 1 65
; The individual arguments fit, but the last selected bin does not.
i 1 0 .01 2 63
</CsScore>
</CsoundSynthesizer>
