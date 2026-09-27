<CsTest>
description = "expsegr emits its release endpoint before the note ends"
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
gkLastControl init 0
gkLastAudio init 0

instr Envelope
  ; Reach 16 after four steps, hold until note-off, then release to 1.
  ; No xtratim here: expsegr must keep the note alive until it emits 1.
  kLine expsegr 1, 4/sr, 16, p4/sr, 1
  aLine expsegr 1, 4/sr, 16, p4/sr, 1
  gkLastControl = kLine
  gkLastAudio downsamp aLine
  ; When note-off interrupts the rise at step 2, release starts at 4.
  if p3 == 2/sr then
    kStep timeinstk
    kStep -= 1
    kExpected = (kStep < 2 ? 2^kStep : 4*.25^(min(4, kStep-2)/4))
    if !(abs(kLine-kExpected) < .00001 && abs(gkLastAudio-kExpected) < .00001) then
      printks "Interrupted rise: expected %g, got control=%g audio=%g\n", 0, kExpected, kLine, gkLastAudio
      exitnowk -1
    endif
  endif
endin

instr CheckLastOutput
  if !(abs(i(gkLastControl)-1) < .00001 && abs(i(gkLastAudio)-1) < .00001) then
    prints "Release must end at 1: last control=%g, last audio=%g\n", i(gkLastControl), i(gkLastAudio)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Check a normal release, an immediate release, and an interrupted rise.
i "Envelope" 0 [8/1024] 4
i "CheckLastOutput" .1 .001
i "Envelope" .2 [8/1024] 0
i "CheckLastOutput" .3 .001
i "Envelope" .4 [2/1024] 4
i "CheckLastOutput" .5 .001
e
</CsScore>
</CsoundSynthesizer>
