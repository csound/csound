<CsTest>
description = "Skipping linear envelope reinit preserves the current segment and release"
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

opcode Check, 0, kkS
  kActual, kExpected, SName xin
  if !(abs(kActual - kExpected) < .00001) then
    printks "%s: expected %g, got %g\n", 0, SName, kExpected, kActual
    exitnowk -1
  endif
endop

instr SkipReinit
  kStep init 0
  kSkip init 0
  ; Reinit halfway through the second ramp, after the cursor has moved.
  if kStep == 6 then
    kSkip = 1
    reinit ENVELOPES
  endif
ENVELOPES:
  iFirstDuration = (i(kSkip) == 0 ? 4/sr : p4)
  kLine linseg 0, iFirstDuration, 1, 4/sr, .5, 4/sr, .75
  aLine linseg 0, iFirstDuration, 1, 4/sr, .5, 4/sr, .75
  kBreak linsegb 0, iFirstDuration, 1, 8/sr, .5, 12/sr, .75
  aBreak linsegb 0, iFirstDuration, 1, 8/sr, .5, 12/sr, .75
  kRelease linsegr 0, iFirstDuration, 1, 4/sr, .5, 4/sr, .75, 4/sr, 0
  aRelease linsegr 0, iFirstDuration, 1, 4/sr, .5, 4/sr, .75, 4/sr, 0
  rireturn
  kAudio downsamp aLine
  kAudioBreak downsamp aBreak
  kAudioRelease downsamp aRelease
  ; The three ramps still end at steps 4, 8 and 12 after reinit.
  if kStep < 4 then
    kExpected = kStep/4
  elseif kStep < 8 then
    kExpected = 1-(kStep-4)/8
  else
    kExpected = min(.75, .5+(kStep-8)/16)
  endif
  kReleaseExpected = (kStep < 16 ? kExpected : max(0, .75*(20-kStep)/4))
  Check kLine, kExpected, "linseg control after skipped reinit"
  Check kAudio, kExpected, "linseg audio after skipped reinit"
  Check kBreak, kExpected, "linsegb control after skipped reinit"
  Check kAudioBreak, kExpected, "linsegb audio after skipped reinit"
  Check kRelease, kReleaseExpected, "linsegr control after skipped reinit"
  Check kAudioRelease, kReleaseExpected, "linsegr audio after skipped reinit"
  kStep += 1
endin
</CsInstruments>
<CsScore>
i "SkipReinit" 0 [16/1024] 0
i "SkipReinit" .1 [16/1024] -1
e
</CsScore>
</CsoundSynthesizer>
