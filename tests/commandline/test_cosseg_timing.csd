<CsTest>
description = "cosine envelopes retain their start and full segment durations"
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

instr 1
  iDur = p4/sr
  kCount init 0
  kSeg cosseg 0, iDur, 1, iDur, 0
  aSeg cosseg 0, iDur, 1, iDur, 0
  kBreak cossegb 0, iDur, 1, 2*iDur, 0
  aBreak cossegb 0, iDur, 1, 2*iDur, 0
  kSingle cossegb 0, iDur, 1
  aSingle cossegb 0, iDur, 1
  kRel cossegr 0, iDur, 1, iDur, 0
  aRel cossegr 0, iDur, 1, iDur, 0
  kAudio downsamp aSeg
  kAudioBreak downsamp aBreak
  kAudioSingle downsamp aSingle
  kAudioRel downsamp aRel

  kRise = (1-cos($M_PI*min(kCount/p4,1)))/2
  kExpected = (kCount < p4 ? kRise : (1+cos($M_PI*min((kCount-p4)/p4,1)))/2)
  kRelease release
  kAfter init 0
  kExpectedRel = kRise
  if kRelease == 1 then
    kExpectedRel = (1+cos($M_PI*kAfter/p4))/2
    kAfter += 1
  endif
  if !(abs(kSeg-kExpected) < .00001 && abs(kAudio-kExpected) < .00001 && abs(kBreak-kExpected) < .00001 && abs(kAudioBreak-kExpected) < .00001 && abs(kSingle-kRise) < .00001 && abs(kAudioSingle-kRise) < .00001 && abs(kRel-kExpectedRel) < .00001 && abs(kAudioRel-kExpectedRel) < .00001) then
    printks "cosine envelope mismatch at sample %g, duration %g: seg %g/%g break %g/%g single %g/%g release %g/%g expected %g/%g/%g\n", 0, kCount, p4, kSeg, kAudio, kBreak, kAudioBreak, kSingle, kAudioSingle, kRel, kAudioRel, kExpected, kRise, kExpectedRel
    exitnowk(-1)
  endif
  if kAfter == p4 then
    gkChecks += 1
  endif
  kCount += 1
endin

instr 99
  if i(gkChecks) != 2 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01171875 1
i 1 .03125 .01171875 4
i 99 .0625 .001
</CsScore>
</CsoundSynthesizer>
