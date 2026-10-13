<CsTest>
description = "modal damping starts after the strike when idec exceeds the note duration"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkEnergy[] init 2

instr 1, 2
  if p4 == 1 then
    aSig vibes .8, 440, .5, .5, 1, 0, 0, 2, .1
  else
    aSig marimba .8, 440, .5, .5, 1, 0, 0, 2, .1, 1, 100
  endif
  kRms rms aSig, 100
  kEnergy init 0
  kEnergy += kRms*kRms
  gkEnergy[p1-1] = kEnergy
endin

instr 3
  if !(gkEnergy[1] > .001 && gkEnergy[0] < .5*gkEnergy[1]) then
    printks "modal short note: damped=%g undamped=%g\n", 0, gkEnergy[0], gkEnergy[1]
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 256 -7 0 16 1 239 0 1 0
f 2 0 256 10 1
i 1 0 .05 1
i 2 0 .5 1
i 3 .04 .001
i 1 1 .05 2
i 2 1 .5 2
i 3 1.04 .001
</CsScore>
</CsoundSynthesizer>
