<CsTest>
description = "diskin2 loop crossfade (sync), scalar and array"

[expect]
exit = 0
output = ["diskin2 crossfade OK"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; A short loop is read from a soundfile, the crossfade length is capped at loopLength/2
;
;   iwrap    1        hard wrap, no crossfade
;   iwrap    1000     crossfade (capped)
;   iwrap    1500     crossfade (capped to 882)  -> identical to iwrap=100000
;   iwrap    100000   crossfade (capped to 882)
;   iwrap    128      crossfade (128 frames)       -> differs from the capped ones
;
; The 1500-vs-100000 pair pins the cap to loopLength/2

sr = 44100
ksmps = 64
nchnls = 1
0dbfs = 1

gkRmsXf    init 0
gkDiff     init 0
gkHardHard init 0
gkClampSame init 0
gkClampDiff init 0
gkArrRms   init 0
gkArrDiff  init 0

instr 1
  ;                          pitch    xfade              end
  ;                             start
  aHard   diskin2 "fox.wav", 1, 0.50, 1,      0,0,0,0,0, 0.54
  aXf     diskin2 "fox.wav", 1, 0.50, 1000,   0,0,0,0,0, 0.54
  aHard2  diskin2 "fox.wav", 1, 0.50, 1,      0,0,0,0,0, 0.54
  aBig1   diskin2 "fox.wav", 1, 0.50, 1500,   0,0,0,0,0, 0.54
  aBig2   diskin2 "fox.wav", 1, 0.50, 100000, 0,0,0,0,0, 0.54
  aSmall  diskin2 "fox.wav", 1, 0.50, 128,    0,0,0,0,0, 0.54
  aArr[]  diskin2 "fox.wav", 1, 0.50, 1000,   0,0,0,0,0, 0.54
  aArrH[] diskin2 "fox.wav", 1, 0.50, 1,      0,0,0,0,0, 0.54

  aDiff    = aHard - aXf
  aHH      = aHard - aHard2
  aSame    = aBig1 - aBig2
  aClamp   = aBig1 - aSmall
  aArrDiff = aArrH[0] - aArr[0]

  kRms     rms aXf
  kDiff    rms aDiff
  kHH      rms aHH
  kSame    rms aSame
  kClamp   rms aClamp
  kArrRms  rms aArr[0]
  kArrDiff rms aArrDiff

  gkRmsXf     = (kRms > gkRmsXf ? kRms : gkRmsXf)
  gkDiff      = (kDiff > gkDiff ? kDiff : gkDiff)
  gkHardHard  = (kHH > gkHardHard ? kHH : gkHardHard)
  gkClampSame = (kSame > gkClampSame ? kSame : gkClampSame)
  gkClampDiff = (kClamp > gkClampDiff ? kClamp : gkClampDiff)
  gkArrRms    = (kArrRms > gkArrRms ? kArrRms : gkArrRms)
  gkArrDiff   = (kArrDiff > gkArrDiff ? kArrDiff : gkArrDiff)
endin

instr 2
  iRms     = i(gkRmsXf)
  iDiff    = i(gkDiff)
  iHH      = i(gkHardHard)
  iSame    = i(gkClampSame)
  iClamp   = i(gkClampDiff)
  iArrRms  = i(gkArrRms)
  iArrDiff = i(gkArrDiff)

  if (iRms <= 0.00001 || iDiff <= 0.00001) then
    prints "diskin2 crossfade scalar failed: rms=%g diff=%g\n", iRms, iDiff
    exitnow(1)
  endif
  if (iHH > 0.000001) then
    prints "diskin2 hard wrap not deterministic: hh=%g\n", iHH
    exitnow(1)
  endif
  if (iSame > 0.000001 || iClamp <= 0.00001) then
    prints "diskin2 crossfade clamp failed: same=%g clamp=%g\n", iSame, iClamp
    exitnow(1)
  endif
  if (iArrRms <= 0.00001 || iArrDiff <= 0.00001) then
    prints "diskin2 crossfade array failed: rms=%g diff=%g\n", iArrRms, iArrDiff
    exitnow(1)
  endif

  prints "diskin2 crossfade OK\n"
endin
</CsInstruments>
<CsScore>
i1 0 0.6
i2 0.7 0
e 0.8
</CsScore>
</CsoundSynthesizer>
