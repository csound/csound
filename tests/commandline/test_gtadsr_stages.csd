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

; Compare all overloads against piecewise linear curves at one sample per cycle.
instr 1
 setksmps 1
 iAttack = max(1, int(p4))
 iDecay = max(1, int(p5))
 iHigh = (p7 > 0 ? p7 : iAttack + iDecay + 2)
 kStep init -1
 kExpected init 0
 kRestart init 0
 kGate = (kStep >= 0 && kStep != iHigh && kStep < iHigh + 8 ? 1 : 0)
 kAttack = (kStep == 0 ? p4/sr : 1000)
 kDecay = (kStep == 0 ? p5/sr : 1000)
 kSustain = p6
 kRelease = log(.001)/log(.5)/sr
 if kStep > iHigh then
  kSustain = .25
  if kStep == iHigh + 1 then
   kAttack = 2/sr
   kDecay = 3/sr
  endif
 endif
 if kStep >= iHigh + 8 then
  kRelease = p8
 endif
 kAmp = 1 + (kStep + 1)*.125
 kEnv gtadsr kAmp, kAttack, kDecay, kSustain, kRelease, kGate
 aEnv gtadsr kAmp, kAttack, kDecay, kSustain, kRelease, kGate
 aInput = 2*kAmp
 aProcessed gtadsr aInput, kAttack, kDecay, kSustain, kRelease, kGate
 kAudio downsamp aEnv
 kProcessed downsamp aProcessed

 if kStep < 0 then
  kExpected = 0
 elseif kStep < iHigh then
  if kStep < iAttack then
   kExpected = (kStep + 1)/iAttack
  elseif kStep < iAttack + iDecay then
   kExpected = 1 + (p6 - 1)*(kStep - iAttack + 1)/iDecay
  else
   kExpected = p6
  endif
 elseif kStep == iHigh then
  kExpected *= .5
  kRestart = kExpected
 elseif kStep < iHigh + 3 then
  kExpected = kRestart + (1-kRestart)*(kStep-iHigh)/2
 elseif kStep < iHigh + 6 then
  kExpected = 1 - .75*(kStep-iHigh-2)/3
 elseif kStep < iHigh + 8 then
  kExpected = .25
 else
  kExpected = 0
 endif
 if !(abs(kEnv-kExpected*kAmp) < .00001 && abs(kAudio-kExpected*kAmp) < .00001 && abs(kProcessed-2*kExpected*kAmp) < .00002) then
  printks "gtadsr curve: attack=%g decay=%g sustain=%g step=%g expected=%g k=%g a=%g processor=%g\n", 0, p4, p5, p6, kStep, kExpected*kAmp, kEnv, kAudio, kProcessed
  exitnowk -1
 endif
 if kStep == iHigh + 9 then
  gkChecks += 1
  turnoff
 endif
 kStep += 1
endin

; Audio processing must cross stage boundaries within a block and advance only
; over active samples. The control-rate envelope uses the same durations.
instr 2
 kCycle init 0
 kGate = (kCycle < 8 ? 1 : 0)
 kRelease = (kCycle == 8 ? 48/sr : 24/sr)
 kEnv gtadsr 1, 64/sr, 64/sr, .5, kRelease, kGate
 aEnv gtadsr 1, 64/sr, 64/sr, .5, kRelease, kGate
 aInput phasor 1
 aInput = 1 + aInput
 aProcessed gtadsr aInput, 64/sr, 64/sr, .5, kRelease, kGate
 kStart offsetsmps
 kEnd init 16
 kCount init 0
 kReleaseStart init 0
 kReleaseAge init 0
 kPrevRelease init 0
 if kCycle == 10 then
  kEnd = p5
 endif
 kIndex = 0
 while kIndex < 16 do
  kActual vaget kIndex, aEnv
  kProcessed vaget kIndex, aProcessed
  kInput vaget kIndex, aInput
  if kIndex < kStart || kIndex >= kEnd then
   kExpected = 0
  else
   if kGate == 1 then
    if kCount < 64 then
     kExpected = (kCount+1)/64
    elseif kCount < 128 then
     kExpected = 1 - .5*(kCount-63)/64
    else
     kExpected = .5
    endif
   else
    if kRelease != kPrevRelease then
     kReleaseStart = kExpected
     kReleaseAge = 0
     kPrevRelease = kRelease
    endif
    kReleaseAge += 1
    kExpected = kReleaseStart*pow(.001, kReleaseAge/(kRelease*sr))
   endif
   kCount += 1
  endif
  if !(abs(kActual-kExpected) < .00001 && abs(kProcessed-kExpected*kInput) < .00002) then
   printks "gtadsr block: offset=%g cycle=%g index=%g expected=%g actual=%g processor=%g\n", 0, p4, kCycle, kIndex, kExpected, kActual, kProcessed
   exitnowk -1
  endif
  kIndex += 1
 od
 if kCycle < 4 then
  kRef = (kCycle+1)/4
 elseif kCycle < 8 then
  kRef = 1 - .5*(kCycle-3)/4
 elseif kCycle == 8 then
  kRef = .05
 else
  kRef = .05*pow(.001, (kCycle-8)/1.5)
 endif
 if !(abs(kEnv-kRef) < .00001) then
  printks "gtadsr control curve: cycle=%g expected=%g actual=%g\n", 0, kCycle, kRef, kEnv
  exitnowk -1
 endif
 if kCycle == 10 then
  gkChecks += 1
 endif
 kCycle += 1
endin

instr 99
 if i(gkChecks) != 13 then
  prints "gtadsr: not all checks ran\n"
  exitnow -1
 endif
endin
</CsInstruments>
<CsScore>
; Attack, decay, sustain, optional early gate-off, final release.
i 1 0 .1 4 4 .5 0 0
i 1 0 .1 1 1 .5 0 0
i 1 0 .1 0 0 .5 0 0
i 1 0 .1 .5 .5 .5 0 0
i 1 0 .1 4 4 0 0 0
i 1 0 .1 4 4 1 0 0
i 1 0 .1 4 4 .5 2 0
i 1 0 .1 4 4 .5 6 0
i 1 0 .1 4 4 .5 0 -1
; Eleven blocks, including a five-sample start offset and partial last blocks.
i 2 .25 .021484375 0 16
i 2 .375 .0206298828125 0 9
i 2 .5006103515625 .0208740234375 5 16
i 2 .6256103515625 .02001953125 5 9
i 99 .75 .001
e
</CsScore>
</CsoundSynthesizer>
