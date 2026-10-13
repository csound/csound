<CsTest>
description = "Linear envelopes cross zero-step segments without delaying later segments"
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

instr MiddleJumps
  ; Both rates have the same time step. p4 is zero or a positive
  ; duration too short for one step. Two jumps must finish at step 4.
  iShort = p4/sr
  kLine linseg 0, 4/sr, 1, iShort, .5, iShort, .25, 4/sr, .75
  aLine linseg 0, 4/sr, 1, iShort, .5, iShort, .25, 4/sr, .75
  kBreak linsegb 0, 4/sr, 1, (4+p4)/sr, .5, (4+2*p4)/sr, .25, (8+2*p4)/sr, .75
  aBreak linsegb 0, 4/sr, 1, (4+p4)/sr, .5, (4+2*p4)/sr, .25, (8+2*p4)/sr, .75
  kRelease linsegr 0, 4/sr, 1, iShort, .5, iShort, .25, 4/sr, .75, 4/sr, 0
  aRelease linsegr 0, 4/sr, 1, iShort, .5, iShort, .25, 4/sr, .75, 4/sr, 0
  kAudio downsamp aLine
  kAudioBreak downsamp aBreak
  kAudioRelease downsamp aRelease
  kStep timeinstk
  kStep -= 1
  ; Ramp 0 -> 1, jump to .25, ramp to .75, then hold.
  if kStep < 4 then
    kExpected = kStep/4
  elseif kStep < 8 then
    kExpected = .25+(kStep-4)/8
  else
    kExpected = .75
  endif
  kReleaseExpected = (kStep < 12 ? kExpected : max(0, .75*(16-kStep)/4))
  Check kLine, kExpected, "linseg control"
  Check kAudio, kExpected, "linseg audio"
  Check kBreak, kExpected, "linsegb control"
  Check kAudioBreak, kExpected, "linsegb audio"
  Check kRelease, kReleaseExpected, "linsegr control"
  Check kAudioRelease, kReleaseExpected, "linsegr audio"
endin

instr EndJumps
  ; A short first segment jumps to 1 on the first step. The final
  ; short segment jumps to .25 at step 4 and holds that value.
  kLine linseg 0, .125/sr, 1, 4/sr, .5, .125/sr, .25
  aLine linseg 0, .125/sr, 1, 4/sr, .5, .125/sr, .25
  kAudio downsamp aLine
  kStep timeinstk
  kExpected = (kStep <= 4 ? 1-(kStep-1)/8 : .25)
  Check kLine, kExpected, "linseg first and last jump, control"
  Check kAudio, kExpected, "linseg first and last jump, audio"
endin

instr InstantRelease
  ; Keep the note alive after its zero-length release so the final
  ; value must remain at zero on every later step.
  xtratim 4/sr
  kLine linsegr 0, 4/sr, 1, 0, 0
  aLine linsegr 0, 4/sr, 1, 0, 0
  kAudio downsamp aLine
  kStep timeinstk
  kStep -= 1
  kExpected = (kStep < 8 ? min(1, kStep/4) : 0)
  Check kLine, kExpected, "instant release, control"
  Check kAudio, kExpected, "instant release, audio"
endin
</CsInstruments>
<CsScore>
i "MiddleJumps" 0 [12/1024] 0
i "MiddleJumps" .1 [12/1024] .125
i "EndJumps" .2 [8/1024]
i "InstantRelease" .3 [8/1024]
e
</CsScore>
</CsoundSynthesizer>
