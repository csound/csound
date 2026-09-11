<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
giShort ftgen 0, 0, 16, -2, 0
giValid ftgen 0, 0, -33, -2, 0
instr 1
  aInput = 0
  fInput pvsanal aInput, 64, 16, 64, 1
  iBuffer, kTime pvsbuffer fInput, p4
endin
instr 2
  fRead pvsbufread 0, p4
endin
instr 3
  aInput = 0
  fInput pvsanal aInput, 64, 16, 64, 1
  iBuffer, kTime pvsbuffer fInput, .125
  fRead pvsbufread2 kTime, iBuffer, (p4 == 0 ? giShort : giValid), (p4 == 0 ? giValid : giShort)
endin
instr 4, 5
  aInput = 0
  fInput pvsanal aInput, 64, 16, 64, 1
  iBuffer, kTime pvsbuffer fInput, .125
  kHandle init iBuffer
  if p1 == 4 then
    fRead pvsbufread kTime, kHandle
  else
    fRead pvsbufread2 kTime, kHandle, giValid, giValid
  endif
  kHandle = -1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 -.1
i 1 0 .01 .0001
i 1 0 .01 1e20
i 2 0 .01 -1
i 2 0 .01 1e20
i 3 .1 .01 0
i 3 .1 .01 1
i 4 .2 .01
i 5 .2 .01
e
</CsScore>
</CsoundSynthesizer>
