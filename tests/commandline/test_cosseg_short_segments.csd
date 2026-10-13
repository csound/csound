<CsTest>
description = "cosine envelopes continue past segments that take no output steps"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Check, 0, kkS
  kActual, kExpected, SName xin
  if !(abs(kActual - kExpected) < 0.00001) then
    printks "%s: expected %g, got %g\n", 0, SName, kExpected, kActual
    exitnowk(-1)
  endif
endop

instr ShortMiddle
  ; Two jumps between four-sample ramps must not discard the last ramp.
  ; p4 is either zero or a positive fraction of one sample/control step.
  iShort = p4/sr
  iRamp = 4/sr
  kSample init 0
  kSeg cosseg 0, iRamp, 1, iShort, 0.5, iShort, 0.25, iRamp, 0.75
  aSeg cosseg 0, iRamp, 1, iShort, 0.5, iShort, 0.25, iRamp, 0.75
  kBreak cossegb 0, iRamp, 1, iRamp+iShort, 0.5, iRamp+2*iShort, 0.25, 2*iRamp+2*iShort, 0.75
  aBreak cossegb 0, iRamp, 1, iRamp+iShort, 0.5, iRamp+2*iShort, 0.25, 2*iRamp+2*iShort, 0.75
  kRel cossegr 0, iRamp, 1, iShort, 0.5, iShort, 0.25, iRamp, 0.75, iRamp, 0
  aRel cossegr 0, iRamp, 1, iShort, 0.5, iShort, 0.25, iRamp, 0.75, iRamp, 0
  kAudio downsamp aSeg
  kAudioBreak downsamp aBreak
  kAudioRel downsamp aRel

  if kSample < 4 then
    kExpected = (1-cos($M_PI*kSample/4))/2
  elseif kSample < 8 then
    kExpected = 0.25 + 0.5*(1-cos($M_PI*(kSample-4)/4))/2
  else
    kExpected = 0.75
  endif
  Check kSeg, kExpected, "cosseg: short middle, k-rate"
  Check kAudio, kExpected, "cosseg: short middle, audio"
  Check kBreak, kExpected, "cossegb: close breakpoints, k-rate"
  Check kAudioBreak, kExpected, "cossegb: close breakpoints, audio"

  ; p5 gives the value reached just before note-off. Also check note-off
  ; during the last ramp, before it reaches its sustain value of 0.75.
  kRelease release
  kAfter init 0
  kExpectedRel = kExpected
  if kRelease == 1 then
    kExpectedRel = p5*(1+cos($M_PI*kAfter/4))/2
    kAfter += 1
  endif
  Check kRel, kExpectedRel, "cossegr: short middle, k-rate"
  Check kAudioRel, kExpectedRel, "cossegr: short middle, audio"
  if kAfter == 4 then
    gkChecks += 1
  endif
  kSample += 1
endin

instr ShortEnds
  ; The first positive duration rounds to zero. The last segment is a jump.
  iShort = 0.125/sr
  iRamp = 4/sr
  ; Keep the note alive long enough to inspect an instantaneous release.
  xtratim iRamp
  kSample init 0
  kSeg cosseg 0, iShort, 1, iRamp, 0.5, iShort, 0
  aSeg cosseg 0, iShort, 1, iRamp, 0.5, iShort, 0
  kBreak cossegb 0, iShort, 1, iShort+iRamp, 0.5, 2*iShort+iRamp, 0
  aBreak cossegb 0, iShort, 1, iShort+iRamp, 0.5, 2*iShort+iRamp, 0
  kRel cossegr 0, iShort, 1, iRamp, 0.5, iShort, 0
  aRel cossegr 0, iShort, 1, iRamp, 0.5, iShort, 0
  kAudio downsamp aSeg
  kAudioBreak downsamp aBreak
  kAudioRel downsamp aRel
  kExpected = (kSample < 4 ? 0.75+0.25*cos($M_PI*kSample/4) : 0)
  Check kSeg, kExpected, "cosseg: short ends, k-rate"
  Check kAudio, kExpected, "cosseg: short ends, audio"
  Check kBreak, kExpected, "cossegb: short ends, k-rate"
  Check kAudioBreak, kExpected, "cossegb: short ends, audio"
  kRelease release
  kExpectedRel = (kSample < 4 ? kExpected : (kRelease == 1 ? 0 : 0.5))
  Check kRel, kExpectedRel, "cossegr: short ends, k-rate"
  Check kAudioRel, kExpectedRel, "cossegr: short ends, audio"
  if kSample == 11 then
    gkChecks += 1
  endif
  kSample += 1
endin

instr CheckCompleted
  if i(gkChecks) != 4 then
    prints "cosine envelope checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i "ShortMiddle" 0 [12/1024] 0 .75
i "ShortMiddle" .03125 [12/1024] .125 .75
i "ShortEnds" .0625 [8/1024]
; Note-off follows phase 1/4 of the last ramp: .25 + .25*(1-cos(pi/4)).
i "ShortMiddle" .09375 [6/1024] .125 .3232233047033631
i "CheckCompleted" .125 .001
</CsScore>
</CsoundSynthesizer>
