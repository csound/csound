<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
gkChecks init 0
giNoFade ftgen 0, 0, -8, -2, 1, 2, 3, 4, 5, 6, 7, 8
giFade ftgen 0, 0, -8, -2, 3, 4, 5, 6, 7, 8, 9, 6
giFullFade ftgen 0, 0, -8, -2, 9, 9, 9, 9, 9, 9, 9, 9

; Interrupt recording during each fade and in the middle, or stop playback.
; A new capture must match a fresh instance recording the same input.
instr 1
 setksmps 1
 kCycle init 0
 kCycle += 1
 aInput = 1
 kOn = (kCycle <= p4 || kCycle >= p4+3 ? 1 : 0)
 kFreshOn = (kCycle >= p4+3 ? 1 : 0)
 aRestart, kRec sndloop aInput, 1, kOn, 32/sr, 8/sr
 aFresh, kFreshRec sndloop aInput, 1, kFreshOn, 32/sr, 8/sr
 kRestart downsamp aRestart
 kFresh downsamp aFresh
 if kCycle >= p4+3 then
  if !(abs(kRestart-kFresh) < .00001) || kRec != kFreshRec then
   printks "sndloop restart mismatch at sample %g: %g / %g\n", 0, kCycle, kRestart, kFresh
   exitnowk(-1)
  endif
 endif
 if kCycle == 200 then
  gkChecks += 1
 endif
endin

; Capture the ramp 1, 2, ... with an eight-sample loop. The tables above
; give the exact playback order, starting just after the recorded fade.
instr 2
 setksmps 1
 kCycle init 0
 kPosition init 0
 kCycle += 1
 aInput = kCycle
 aLoop, kRec sndloop aInput, p4, 1, 8/sr, p5/sr
 kLoop downsamp aLoop
 if kCycle <= 8+p5 then
  kExpected = kCycle
 else
  iTable = (p5 == 0 ? giNoFade : (p5 == 2 ? giFade : giFullFade))
  kExpected table kPosition, iTable
  kPosition += p4
  kPosition -= floor(kPosition/8)*8
 endif
 kExpectedRec = (kCycle < 8+p5 ? 1 : 0)
 if !(abs(kLoop-kExpected) < .00001) || kRec != kExpectedRec then
  printks "sndloop playback mismatch: pitch=%g fade=%g sample=%g: %g / %g, recording=%g / %g\n", 0, p4, p5, kCycle, kLoop, kExpected, kRec, kExpectedRec
  exitnowk(-1)
 endif
 if kCycle == 200 then
  gkChecks += 1
 endif
endin

opcode LoopSample, aa, ak
 setksmps 1
 aInput, kOn xin
 aLoop, kRec sndloop aInput, -.5, kOn, 16/sr, 4/sr
 aRec upsamp kRec
 xout aLoop, aRec
endop

; Block size and inactive samples must not alter recording or playback.
instr 3
 kCycle init 0
 kCycle += 1
 aInput oscili .5, 700
 kOn = (kCycle % 5 == 0 ? 0 : 1)
 aLoop, kRec sndloop aInput, -.5, kOn, 16/sr, 4/sr
 aReference, aRefRec LoopSample aInput, kOn
 kIndex = 0
 while kIndex < ksmps do
  kLoop vaget kIndex, aLoop
  kReference vaget kIndex, aReference
  if !(abs(kLoop-kReference) < .00001) then
   printks "sndloop block mismatch at cycle %g sample %g: %g / %g\n", 0, kCycle, kIndex, kLoop, kReference
   exitnowk(-1)
  endif
  kIndex += 1
 od
 kEarly earlysmps
 kReferenceRec vaget ksmps-kEarly-1, aRefRec
 if kRec != kReferenceRec then
  printks "sndloop recording status differs between block sizes\n", 0
  exitnowk(-1)
 endif
 if kCycle == 1 then
  gkChecks += 1
 endif
endin

; A one-sample loop must always read its sole sample, at any playback step.
instr 4
 setksmps 1
 kCycle init 0
 kCycle += 1
 aInput = kCycle
 aLoop, kRec sndloop aInput, p4, 1, 1/sr, p5/sr
 kLoop downsamp aLoop
 kExpected = (kCycle <= 1+p5 ? kCycle : 1+p5)
 kExpectedRec = (kCycle < 1+p5 ? 1 : 0)
 if !(abs(kLoop-kExpected) < .00001) || kRec != kExpectedRec then
  printks "sndloop one-sample loop mismatch: pitch=%g fade=%g sample=%g: %g / %g\n", 0, p4, p5, kCycle, kLoop, kExpected
  exitnowk(-1)
 endif
 if kCycle == 200 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 22 then
  prints "sndloop checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 4
i 1 0 .03125 20
i 1 0 .03125 36
i 1 0 .03125 60
i 2 0 .03125 1 2
i 2 0 .03125 -1 2
i 2 0 .03125 .5 2
i 2 0 .03125 -.5 2
i 2 0 .03125 0 2
i 2 0 .03125 16 2
i 2 0 .03125 -16 2
i 2 0 .03125 8000001 2
i 2 0 .03125 -8000001 2
i 2 0 .03125 1 0
i 2 0 .03125 1 8
; Reuse an instance after its first capture has ended.
i 2 .0625 .03125 1 2
i 3 0 .125
i 3 .0006103515625 .029296875
i 3 .002197265625 .00244140625
i 4 0 .03125 -1e-30 0
i 4 0 .03125 1e30 0
i 4 0 .03125 -1 1
i 99 .14 .01
e
</CsScore>
</CsoundSynthesizer>
