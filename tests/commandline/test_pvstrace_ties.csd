<CsTest>
description = "pvstrace keeps the requested count with equal amplitudes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckTies
 ; Bin 5 is strongest. Bins 2, 3 and 4 tie for the second place:
 ; choose bin 2, regardless of the optional output-list sorting.
 kInput[] fillarray 0,0, 0,512, .5,1024, .5,1536, .5,2048, .8,2560, 0,3072, 0,3584, 0,4096
 kSimple[] init 18
 kListed[] init 18
 fInput pvsfromarray kInput, 4
 fSimple pvstrace fInput, 2
 fListed, kBins[] pvstrace fInput, 2, p4
 kFrame pvs2array kSimple, fSimple
 kListedFrame pvs2array kListed, fListed
 kPrevious init -1
 kSeen init 0
 if kFrame != kPrevious && kFrame > 0 then
  kPrevious = kFrame
  kSeen += 1
  kBin = 0
  while kBin < 9 do
   kAmplitude = (kBin == 2 || kBin == 5 ? kInput[2*kBin] : 0)
   kFrequency = (kBin == 2 || kBin == 5 ? kInput[2*kBin+1] : 0)
   kError = abs(kSimple[2*kBin]-kAmplitude) + abs(kListed[2*kBin]-kAmplitude) + abs(kSimple[2*kBin+1]-kFrequency) + abs(kListed[2*kBin+1]-kFrequency)
   if !(kError < 1e-6) then
    printks "pvstrace tie: frame=%g bin=%g error=%g\n", 0, kFrame, kBin, kError
    exitnowk(-1)
   endif
   kBin += 1
  od
  iFirst = (p4 == 0 ? 2 : 5)
  iSecond = (p4 == 0 ? 5 : 2)
  if !(kBins[0] == iFirst && kBins[1] == iSecond && sumarray(kBins) == iFirst+iSecond) then
   printks "pvstrace tied bin list: %g,%g\n", 0, kBins[0], kBins[1]
   exitnowk(-1)
  endif
  if kSeen == 8 then
   gkChecks += 1
  endif
 endif
endin

instr CheckResults
 if i(gkChecks) != 2 then
  prints "pvstrace tie checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckTies" 0 .02 0
i "CheckTies" 0 .02 1
i "CheckResults" .03 .01
e
</CsScore>
</CsoundSynthesizer>
