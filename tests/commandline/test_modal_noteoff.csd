<CsTest>
description = "vibes and marimba damp indefinite notes on note-off"

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
gkLevel[] init 2
gkPeak[] init 2

instr 1, 2
  if p4 == 1 then
    aSig vibes .8, 440, .5, .5, 1, 0, 0, 2, .1
  else
    ; Force three strikes so both instances receive the same excitation.
    aSig marimba .8, 440, .5, .5, 1, 0, 0, 2, .1, 1, 100
  endif
  kLevel rms aSig, 100
  kPeak peak aSig
  gkLevel[p1-1] = kLevel
  gkPeak[p1-1] = kPeak
endin

instr 3
  if !(gkPeak[0] > .001 && gkPeak[1] > .001 && abs(gkLevel[0]-gkLevel[1]) < .00001) then
    printks "modal note-off: scheduled=%g released=%g\n", 0, gkLevel[0], gkLevel[1]
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 256 -7 0 16 1 239 0 1 0
f 2 0 256 10 1
; Scheduled damping at .5 seconds, compared with note-off at .5 seconds.
i 1 0 .6 1
i 2 0 -1 1
i -2 .5 0
i 3 .55 .01
i 1 1 .6 2
i 2 1 -1 2
i -2 1.5 0
i 3 1.55 .01
</CsScore>
</CsoundSynthesizer>
