<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kOrder init 2
 kCount init 0
 kX[] init 4
 kY[] init 4
 kN1[] init 4
 kN2[] init 4
 kFeedback1 init 0
 kFeedback2 init 0
 kCount += 1
 kInput = .1 + .02*sin(kCount*.31)
 aInput = kInput
 ; Exercise unchanged, larger and smaller orders, then restore stages
 ; whose storage remained allocated while the order was smaller.
 kReinit = kCount == 40 || kCount == 80 || kCount == 120 || kCount == 160 || kCount == 200
 if kReinit != 0 then
  if kCount == 80 || kCount == 200 then
   kOrder = 4
  elseif kCount == 160 then
   kOrder = 2
  endif
  reinit FILTERS
 endif
FILTERS:
 iOrder = i(kOrder)
 ; Start each score note from zero even if Csound recycles its instance.
 iSkip = i(kCount) == 0 ? 0 : p4
 aOne phaser1 aInput, 700, iOrder, p6, iSkip
 aTwo phaser2 aInput, 500, 1, iOrder, p5, 1.5, p6, iSkip
 rireturn

 ; Independent sample recurrences retain each existing stage for iskip=1
 ; and clear all history, including feedback, for iskip=0.
 if kReinit != 0 && p4 == 0 then
  kJ = 0
  while kJ < 4 do
   kX[kJ] = 0
   kY[kJ] = 0
   kN1[kJ] = 0
   kN2[kJ] = 0
   kJ += 1
  od
  kFeedback1 = 0
  kFeedback2 = 0
 endif
 kBeta = (1-$M_PI*700/sr)/(1+$M_PI*700/sr)
 kOne = kInput + p6*kFeedback1
 kTwo = kInput + p6*kFeedback2
 kJ = 0
 while kJ < kOrder do
  kNext = kBeta*(kOne+kY[kJ])-kX[kJ]
  kX[kJ] = kOne
  kY[kJ] = kNext
  kOne = kNext
  kFrequency = p5 == 1 ? 500*(1+1.5*kJ) : 500*1.5^kJ
  kRadius = exp(-kFrequency*$M_PI/sr)
  kB = -2*kRadius*cos(kFrequency*2*$M_PI/sr)
  kA = kRadius*kRadius
  kTemp = kTwo-kB*kN1[kJ]-kA*kN2[kJ]
  kTwo = kA*kTemp+kB*kN1[kJ]+kN2[kJ]
  kN2[kJ] = kN1[kJ]
  kN1[kJ] = kTemp
  kJ += 1
 od
 kFeedback1 = kOne
 kFeedback2 = kTwo
 kActualOne downsamp aOne
 kActualTwo downsamp aTwo
 if !(abs(kActualOne-kOne) < 1e-5) || !(abs(kActualTwo-kTwo) < 1e-5) then
  printks "phaser reinit mismatch: skip=%g mode=%g sample=%g first=%g/%g second=%g/%g\n", 0, p4, p5, kCount, kActualOne, kOne, kActualTwo, kTwo
  exitnowk(-1)
 endif
 if kCount == 240 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 4 then
  prints "phaser reinit checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .031 0 1 .7
i 1 .04 .031 1 1 .7
i 1 .08 .031 0 2 -.7
i 1 .12 .031 1 2 -.7
i 99 .16 .001
e
</CsScore>
</CsoundSynthesizer>
