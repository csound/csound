<CsTest>
description = "randc preserves its phase and random history when a negative seed skips reinitialization"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckReinit
  aReference randc 1, 4, .5, p4
  kReference randc 1, 4, .5, p4
  kSample init 0
  kSkip init 0
  if kSample == 3 || kSample == 11 then
    kSkip = 1
    reinit GENERATORS
  endif
GENERATORS:
  iSeed = (i(kSkip) == 0 ? .5 : -1)
  aActual randc 1, 4, iSeed, p4
  kActual randc 1, 4, iSeed, p4
  rireturn
  kAudio downsamp aActual
  kAudioReference downsamp aReference
  if !(abs(kActual-kReference) < .00001 && abs(kAudio-kAudioReference) < .00001) then
    printks "randc size=%g sample=%g: uninterrupted=%g/%g reinitialized=%g/%g\n", \
      0, p4, kSample, kReference, kAudioReference, kActual, kAudio
    exitnowk -1
  endif
  kSample += 1
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 32 then
    prints "randc reinit checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckReinit" 0 .5 0
i "CheckReinit" 0 .5 1
i "CheckResults" .5 .03125
e
</CsScore>
</CsoundSynthesizer>
