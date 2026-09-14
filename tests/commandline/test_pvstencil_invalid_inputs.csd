<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
giShort ftgen 1, 0, 8, -2, 0
giMask ftgen 2, 0, 64, -2, 0
instr 1
  aInput init 0
  fInput pvsanal aInput, 64, p5, 64, 1
  fOutput pvstencil fInput, .5, 1, p4
endin
instr 2
  fInput pvsinit 64, 16, 64, 1, 2
  fOutput pvstencil fInput, .5, 1, giMask
endin
</CsInstruments>
<CsScore>
i 1 0 .01 999 16
i 1 0 .01 999 1
i 1 0 .01 1 16
i 1 0 .01 1 1
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
