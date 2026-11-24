<CsTest>
description = "phaser2 preserves canonical history when float parameters change"
[expect]
exit = 0
</CsTest>
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
  iOrder = p4
  iMode = p5
  iOffset = int(p2*sr+.5) % ksmps
  kCount init 0
  kN1[] init 4
  kN2[] init 4
  kFeedback init 0
  kFrequency = (kCount % 48 < 16 ? 1000 : (kCount % 48 < 32 ? 3000 : 5000))
  kQ = .5+(kCount % 5)*.25
  kSeparation = (kCount % 3 == 0 ? -.5 : 1.5)
  kGain = (kCount % 3 == 0 ? -.5 : .5)
  aInput oscili .5, 321
  aInput += .1
  aOutput phaser2 aInput, -kFrequency, kQ, iOrder, iMode, kSeparation, kGain
  aReuse = aInput
  aReuse phaser2 aReuse, -kFrequency, kQ, iOrder, iMode, kSeparation, kGain
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps,kStart+128-kCount)
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kInput vaget kN, aInput
      kValue = kInput+kGain*kFeedback
      kJ = 0
      while kJ < iOrder do
        kF = (iMode == 1 ? kFrequency*(1+abs(kSeparation)*kJ) : kFrequency*abs(kSeparation)^kJ)
        kRadius = exp(-$M_PI*kF/(sr*kQ))
        kB = -2*kRadius*cos(2*$M_PI*kF/sr)
        kA = kRadius*kRadius
        kTemp = kValue-kB*kN1[kJ]-kA*kN2[kJ]
        kValue = kA*kTemp+kB*kN1[kJ]+kN2[kJ]
        kN2[kJ] = kN1[kJ]
        kN1[kJ] = kTemp
        kJ += 1
      od
      kExpected = (iOrder == 0 ? 0 : kValue)
      kFeedback = kExpected
    endif
    kActual vaget kN, aOutput
    kReuse vaget kN, aReuse
    if !(abs(kActual-kExpected) < .00002*(1+abs(kExpected)) && kActual == kReuse) then
      printks "FAIL phaser2 order=%g mode=%g sample=%g output=%g reuse=%g expected=%g\n", 0, iOrder, iMode, kCount+kN-kStart, kActual, kReuse, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd-kStart
  if kCount == 128 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 16 then
    prints "FAIL phaser2 state checks did not complete: %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 0 1
i 1 0 .015625 1 1
i 1 0 .015625 2 1
i 1 0 .015625 4 1
i 1 0 .015625 0 2
i 1 0 .015625 1 2
i 1 0 .015625 2 2
i 1 0 .015625 4 2
i 1 0.0316162109375 .015625 0 1
i 1 0.0316162109375 .015625 1 1
i 1 0.0316162109375 .015625 2 1
i 1 0.0316162109375 .015625 4 1
i 1 0.0316162109375 .015625 0 2
i 1 0.0316162109375 .015625 1 2
i 1 0.0316162109375 .015625 2 2
i 1 0.0316162109375 .015625 4 2
i 99 .0625 .001
e
</CsScore>
</CsoundSynthesizer>
