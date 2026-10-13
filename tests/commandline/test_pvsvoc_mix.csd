<CsTest>
description = "pvsvoc interpolates between the dry and wet spectra as controls change"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kCycle init 0
 kCycle += 1
 ; Exercise dry output, a half mix, and fully wet output on the same instance.
 kDepth = kCycle <= 12 ? 0 : (kCycle <= 24 ? .5 : 1)
 kCoefs = kCycle <= 12 ? 20 : (kCycle <= 24 ? 0 : 1e20)
 ; Zero requests the documented default. A count above the transform size
 ; keeps every coefficient, just as a count of 64 does for this spectrum.
 kReferenceCoefs = kCycle <= 12 ? 20 : (kCycle <= 24 ? 80 : 64)
 kAmplitude[] init 130
 kExcitation[] init 130
 kBin = 0
 while kBin < 65 do
  kAmplitude[2*kBin] = (1+kBin%4)/8
  kAmplitude[2*kBin+1] = kBin*sr/128
  kExcitation[2*kBin] = (1+kBin%5)/16
  kExcitation[2*kBin+1] = kBin*sr/128*.75
  kBin += 1
 od
 fAmplitude pvsfromarray kAmplitude, 32
 fExcitation pvsfromarray kExcitation, 32
 fWet pvsvoc fAmplitude, fExcitation, 1, 2, kReferenceCoefs
 fMixed pvsvoc fAmplitude, fExcitation, kDepth, 2, kCoefs
 kWet[] init 130
 kMixed[] init 130
 kWetFrame pvs2array kWet, fWet
 kMixedFrame pvs2array kMixed, fMixed

 kPreviousFrame init 0
 if kMixedFrame > kPreviousFrame then
  kBin = 0
  while kBin < 65 do
   kExpectedAmplitude = kAmplitude[2*kBin]*2*(1-kDepth) + kWet[2*kBin]*kDepth
   kExpectedFrequency = kAmplitude[2*kBin+1]*(1-kDepth) + kExcitation[2*kBin+1]*kDepth
   if !(abs(kMixed[2*kBin]-kExpectedAmplitude) < .000001 && abs(kMixed[2*kBin+1]-kExpectedFrequency) < .001) then
    printks "cycle %g, depth %g, bin %g: amplitude=%g expected=%g; frequency=%g expected=%g\n", 0, kCycle, kDepth, kBin, kMixed[2*kBin], kExpectedAmplitude, kMixed[2*kBin+1], kExpectedFrequency
    exitnowk(-1)
   endif
   kBin += 1
  od
  kPreviousFrame = kMixedFrame
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) < 9 then
  prints "pvsvoc control changes were not checked\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .036
i 99 .04 .001
e
</CsScore>
</CsoundSynthesizer>
