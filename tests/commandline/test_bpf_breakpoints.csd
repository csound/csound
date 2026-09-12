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
instr 1
 iXs[] fillarray 0, 1, 2, 3
 iYs[] fillarray 10, 20, -5, 30
 kXs[] fillarray 0, 1, 2, 3
 kYs[] fillarray 10, 20, -5, 30
 kTests[] fillarray 1, 2, 0, 3, -1, 4, 1.25, 1.75, .5, 2.5, 2, 1, 2.75, .25, 0, 3
 kCycle init 0
 kX = kTests[kCycle % 16]
 kCycle += 1
 if kX <= 0 then
  kLinear = 10
  kCosine = 10
 elseif kX >= 3 then
  kLinear = 30
  kCosine = 30
 else
  kSegment = int(kX)
  kFraction = kX-kSegment
  kY0 = kYs[kSegment]
  kY1 = kYs[kSegment+1]
  kLinear = kY0 + (kY1-kY0)*kFraction
  kCosine = kY0 + (kY1-kY0)*(1-cos(kFraction*3.141592653589793))*.5
 endif
 iLinear bpf 1, 0, 10, 1, 20, 2, -5, 3, 30
 iCosine bpfcos 1, 0, 10, 1, 20, 2, -5, 3, 30
 iArrayLinear bpf 1, iXs, iYs
 iArrayCosine bpfcos 1, iXs, iYs
 if iLinear != 20 || iCosine != 20 || iArrayLinear != 20 || iArrayCosine != 20 then
  prints "bpf init-rate breakpoint mismatch\n"
  exitnow(-1)
 endif
 kLinear1 bpf kX, 0, 10, 1, 20, 2, -5, 3, 30
 kCosine1 bpfcos kX, 0, 10, 1, 20, 2, -5, 3, 30
 kLinear2 bpf kX, iXs, iYs
 kCosine2 bpfcos kX, iXs, iYs
 kLinear3 bpf kX, kXs, kYs
 kCosine3 bpfcos kX, kXs, kYs
 kDual1, kDual2 bpf kX, kXs, kYs, kYs
 kInput[] fillarray 0, 0
 kInput[0] = kX
 kInput[1] = kX
 kLinearArray[] bpf kInput, 0, 10, 1, 20, 2, -5, 3, 30
 kCosineArray[] bpfcos kInput, 0, 10, 1, 20, 2, -5, 3, 30
 if !(abs(kLinear1-kLinear)+abs(kLinear2-kLinear)+abs(kLinear3-kLinear)+abs(kDual1-kLinear)+abs(kDual2-kLinear)+abs(kCosine1-kCosine)+abs(kCosine2-kCosine)+abs(kCosine3-kCosine)+abs(kLinearArray[0]-kLinear)+abs(kLinearArray[1]-kLinear)+abs(kCosineArray[0]-kCosine)+abs(kCosineArray[1]-kCosine) < .0001) then
  printks "bpf control-rate mismatch at %g\n", 0, kX
  exitnowk(-1)
 endif
 aX = kX
 aLinear1 bpf aX, 0, 10, 1, 20, 2, -5, 3, 30
 aCosine1 bpfcos aX, 0, 10, 1, 20, 2, -5, 3, 30
 aLinear2 bpf aX, iXs, iYs
 aCosine2 bpfcos aX, iXs, iYs
 aLinear3 bpf aX, kXs, kYs
 aCosine3 bpfcos aX, kXs, kYs
 aAlias = aX
 aAlias bpf aAlias, 0, 10, 1, 20, 2, -5, 3, 30
 iOffset = round(frac(p2*sr/ksmps)*ksmps)
 iSamples = round(p3*sr)
 kIndex = 0
 while kIndex < ksmps do
  kPosition = (kCycle-1)*ksmps+kIndex-iOffset
  kExpectedLinear = (kPosition >= 0 && kPosition < iSamples ? kLinear : 0)
  kExpectedCosine = (kPosition >= 0 && kPosition < iSamples ? kCosine : 0)
  kLinear1Sample vaget kIndex, aLinear1
  kLinear2Sample vaget kIndex, aLinear2
  kLinear3Sample vaget kIndex, aLinear3
  kCosine1Sample vaget kIndex, aCosine1
  kCosine2Sample vaget kIndex, aCosine2
  kCosine3Sample vaget kIndex, aCosine3
  kAliasSample vaget kIndex, aAlias
  if !(abs(kLinear1Sample-kExpectedLinear)+abs(kLinear2Sample-kExpectedLinear)+abs(kLinear3Sample-kExpectedLinear)+abs(kAliasSample-kExpectedLinear)+abs(kCosine1Sample-kExpectedCosine)+abs(kCosine2Sample-kExpectedCosine)+abs(kCosine3Sample-kExpectedCosine) < .0001) then
   printks "bpf audio-rate mismatch at x=%g sample=%g\n", 0, kX, kIndex
   exitnowk(-1)
  endif
  kIndex += 1
 od
 gkChecks += 1
endin
instr 99
 if i(gkChecks) != 19 then
  prints "bpf processing stopped early: %g checks\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
i 1 .0006103515625 .00244140625
i 1 .001953125 .000732421875
i 99 .03515625 .001953125
e
</CsScore>
</CsoundSynthesizer>
