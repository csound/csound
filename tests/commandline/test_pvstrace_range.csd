<CsTest>
description = "pvstrace searches only the requested bins on every frame"

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

instr CheckRange
 ; Bins 1 and 7 are louder, but outside [2, 6). Inside it, bins 3 and 2
 ; are strongest. Identical frames must keep returning the same selection.
 kInput[] fillarray 0,0, .9,512, .3,1024, .4,1536, .1,2048, .05,2560, 0,3072, .8,3584, 0,4096
 kOutput[] init 18
 fInput pvsfromarray kInput, 4
 fOutput, kBins[] pvstrace fInput, 2, p4, 2, 6
 kFrame pvs2array kOutput, fOutput
 kPrevious init -1
 kSeen init 0
 if kFrame != kPrevious && kFrame > 0 then
  kPrevious = kFrame
  kSeen += 1
  kBin = 0
  while kBin < 9 do
   kExpected = (kBin == 2 || kBin == 3 ? kInput[2*kBin] : 0)
   if !(abs(kOutput[2*kBin]-kExpected) < 1e-6) then
    printks "pvstrace range: frame=%g bin=%g expected=%g actual=%g\n", 0, kFrame, kBin, kExpected, kOutput[2*kBin]
    exitnowk(-1)
   endif
   kBin += 1
  od
  iFirst = (p4 == 0 ? 2 : 3)
  iSecond = (p4 == 0 ? 3 : 2)
  if !(kBins[0] == iFirst && kBins[1] == iSecond && sumarray(kBins) == iFirst+iSecond) then
   printks "pvstrace bin list: expected %g,%g got %g,%g\n", 0, iFirst, iSecond, kBins[0], kBins[1]
   exitnowk(-1)
  endif
  if kSeen == 8 then
   gkChecks += 1
  endif
 endif
endin

instr CheckResults
 if i(gkChecks) != 3 then
  prints "pvstrace range checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckRange" 0 .02 0
i "CheckRange" 0 .02 1
i "CheckRange" 0 .02 -1
i "CheckResults" .03 .01
e
</CsScore>
</CsoundSynthesizer>
