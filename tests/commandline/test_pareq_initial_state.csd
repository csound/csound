<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; First use with iskip must match normal initialization in every mode.
; Check all samples, including inactive parts of a block and reused input.
instr 1
 kCycle init 0
 kCycle += 1
 kFrequency = 512 + (kCycle % 4)*128
 kGain = 1 + (kCycle % 3)
 aInput oscili .5, 731
 aInput += .2
 aNormal pareq aInput, kFrequency, kGain, .8, p4, 0
 aSkipped pareq aInput, kFrequency, kGain, .8, p4, 1
 aAlias = aInput
 aAlias pareq aAlias, kFrequency, kGain, .8, p4, 1
 kIndex = 0
 while kIndex < 16 do
  kNormal vaget kIndex, aNormal
  kSkipped vaget kIndex, aSkipped
  kAlias vaget kIndex, aAlias
  if !(abs(kNormal-kSkipped)+abs(kNormal-kAlias) < .000001) then
   printks "pareq first skip: mode=%g sample=%g normal=%g skipped=%g alias=%g\n", 0, p4, kIndex, kNormal, kSkipped, kAlias
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kCycle == 1 then
  gkChecks += 1
 endif
endin

; A reset takes the new mode and clears history. A skipped reset keeps
; the old mode and history while k-rate parameters continue to update.
instr 2
 setksmps 1
 kCycle init 0
 kCycle += 1
 kFrequency = 512 + (kCycle % 8)*128
 kGain = 1 + (kCycle % 5)
 aInput = (kCycle % 11 - 5)/10
 if kCycle == 19 then
  reinit FILTER
 endif
FILTER:
 iMode = (i(kCycle) >= 19 ? p5 : p4)
 aActual pareq aInput, kFrequency, kGain, .8, iMode, p6
 rireturn
 aContinuous pareq aInput, kFrequency, kGain, .8, p4, 0
 if kCycle >= 19 then
  aFresh pareq aInput, kFrequency, kGain, .8, p5, 0
 endif
 kActual downsamp aActual
 if kCycle >= 19 && p6 == 0 then
  kExpected downsamp aFresh
 else
  kExpected downsamp aContinuous
 endif
 if !(abs(kActual-kExpected) < .000001) then
  printks "pareq reset: mode=%g->%g skip=%g cycle=%g actual=%g expected=%g\n", 0, p4, p5, p6, kCycle, kActual, kExpected
  exitnowk(-1)
 endif
 if kCycle == 64 then
  gkChecks += 1
 endif
endin
instr 99
 if i(gkChecks) != 24 then
  prints "pareq checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 0
i 1 .0042724609375 .0009765625 0
i 1 0 .015625 1
i 1 .0042724609375 .0009765625 1
i 1 0 .015625 2
i 1 .0042724609375 .0009765625 2
i 2 .0625 .0078125 0 0 0
i 2 .0625 .0078125 0 1 0
i 2 .0625 .0078125 0 2 0
i 2 .0625 .0078125 1 0 0
i 2 .0625 .0078125 1 1 0
i 2 .0625 .0078125 1 2 0
i 2 .0625 .0078125 2 0 0
i 2 .0625 .0078125 2 1 0
i 2 .0625 .0078125 2 2 0
i 2 .0625 .0078125 0 0 1
i 2 .0625 .0078125 0 1 1
i 2 .0625 .0078125 0 2 1
i 2 .0625 .0078125 1 0 1
i 2 .0625 .0078125 1 1 1
i 2 .0625 .0078125 1 2 1
i 2 .0625 .0078125 2 0 1
i 2 .0625 .0078125 2 1 1
i 2 .0625 .0078125 2 2 1
i 99 .09375 .001953125
e
</CsScore>
</CsoundSynthesizer>
