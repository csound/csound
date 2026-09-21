<CsTest>
description = "diskin2 crossfade cap accounts for playback speed at the loop boundary"
[expect]
exit = 0
output = ["diskin2 crossfade speed OK"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; Loop 0.50..0.54 s (1764 frames) at kpitch=2, so one output frame advances two
; source frames. iwrap=1000 is capped to loopLength/2 = 882 output frames, which
; at speed 2 would span the whole 1764-frame loop and leave the restart position
; exactly on loopEnd, collapsing the loop. The cap must be measured in source
; frames, and the restart position must stay inside the loop.
;
; aXf must therefore differ from the hard-wrap aHard (crossfade active) and
; must keep varying sample to sample (the loop did not collapse to a constant).

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

gkXf init 0
gkDx init 0
gkDiff init 0
gkArrXf init 0
gkArrDx init 0
gkArrDiff init 0

instr 1
  ;                        pitch start  wrap   fmt win buf skip sync end
  aHard    diskin2 "fox.wav", 2, 0.50, 1,    0, 0, 0, 0, 0, 0.54
  aXf      diskin2 "fox.wav", 2, 0.50, 1000, 0, 0, 0, 0, 0, 0.54
  aArrH[]  diskin2 "fox.wav", 2, 0.50, 1,    0, 0, 0, 0, 0, 0.54
  aArrX[]  diskin2 "fox.wav", 2, 0.50, 1000, 0, 0, 0, 0, 0, 0.54

  aDiff   = aXf - aHard
  aDiffA  = aArrX[0] - aArrH[0]
  aVar    = aXf - delay1(aXf)
  aVarA   = aArrX[0] - delay1(aArrX[0])

  kXf     rms aXf
  kDiff   rms aDiff
  kVar    rms aVar
  kArrXf  rms aArrX[0]
  kArrDf  rms aDiffA
  kArrVar rms aVarA

  gkXf      = (kXf > gkXf ? kXf : gkXf)
  gkDiff    = (kDiff > gkDiff ? kDiff : gkDiff)
  gkDx      = (kVar > gkDx ? kVar : gkDx)
  gkArrXf   = (kArrXf > gkArrXf ? kArrXf : gkArrXf)
  gkArrDiff = (kArrDf > gkArrDiff ? kArrDf : gkArrDiff)
  gkArrDx   = (kArrVar > gkArrDx ? kArrVar : gkArrDx)
endin

instr 2
  iXf = i(gkXf)
  iDiff = i(gkDiff)
  iDx = i(gkDx)
  iArrXf = i(gkArrXf)
  iArrDiff = i(gkArrDiff)
  iArrDx = i(gkArrDx)

  if iXf <= 0.001 || iArrXf <= 0.001 then
    prints "diskin2 crossfade speed silent: xf=%g arr=%g\n", iXf, iArrXf
    exitnow(1)
  endif
  if iDiff <= 0.02 || iArrDiff <= 0.02 then
    prints "diskin2 crossfade speed inactive: diff=%g arr=%g\n", iDiff, iArrDiff
    exitnow(1)
  endif
  if iDx <= 0.001 || iArrDx <= 0.001 then
    prints "diskin2 crossfade speed collapsed: var=%g arr=%g\n", iDx, iArrDx
    exitnow(1)
  endif
  prints "diskin2 crossfade speed OK\n"
endin
</CsInstruments>
<CsScore>
i1 0 0.8
i2 0.9 0
e
</CsScore>
</CsoundSynthesizer>
