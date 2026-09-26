<CsTest>
description = "pvstrace bounds counts and bin ranges before selection"

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

instr CheckBounds
 ; Amplitudes increase with bin number, so the expected selection is a
 ; contiguous range: p7 is its first bin and p8 is its length.
 kInput[] fillarray .1,0, .2,512, .3,1024, .4,1536, .5,2048, .6,2560, .7,3072, .8,3584, .9,4096
 kOutput[] init 18
 kSimple[] init 18
 fInput pvsfromarray kInput, 4
 fOutput, kBins[] pvstrace fInput, p4, 0, p5, p6
 kFrame pvs2array kOutput, fOutput
 if p5 == 0 && p6 == 0 then
  fSimple pvstrace fInput, p4
  kSimpleFrame pvs2array kSimple, fSimple
 endif
 kPrevious init -1
 kSeen init 0
 if kFrame != kPrevious && kFrame > 0 then
  kPrevious = kFrame
  kSeen += 1
  kBin = 0
  while kBin < 9 do
   kExpected = (kBin >= p7 && kBin < p7+p8 ? kInput[2*kBin] : 0)
   kExpectedIndex = (kBin < p8 ? p7+kBin : 0)
   if !(abs(kOutput[2*kBin]-kExpected) < 1e-6 && kBins[kBin] == kExpectedIndex) then
    printks "pvstrace bounds: count=%g min=%g max=%g bin=%g expected=%g actual=%g index=%g\n", 0, p4, p5, p6, kBin, kExpected, kOutput[2*kBin], kBins[kBin]
    exitnowk(-1)
   endif
   if p5 == 0 && p6 == 0 && !(abs(kSimple[2*kBin]-kExpected) < 1e-6) then
    printks "pvstrace simple count: count=%g bin=%g expected=%g actual=%g\n", 0, p4, kBin, kExpected, kSimple[2*kBin]
    exitnowk(-1)
   endif
   kBin += 1
  od
  if kSeen == 8 then
   gkChecks += 1
  endif
 endif
endin

instr CheckResults
 if i(gkChecks) != 13 then
  prints "pvstrace bounds checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Count, lower limit, upper limit, expected first bin, expected bin count.
; Keep the existing minimum count of one, and truncate fractional counts.
i "CheckBounds" 0 .02 -5    0    0    8 1
i "CheckBounds" 0 .02  0    0    0    8 1
i "CheckBounds" 0 .02  1.9  0    0    8 1
i "CheckBounds" 0 .02  3.8  0    0    6 3
i "CheckBounds" 0 .02  9    0    0    0 9
i "CheckBounds" 0 .02  1e20 0    0    0 9
; Limit the count to the search range. The upper bound is exclusive.
i "CheckBounds" 0 .02  8    2    6    2 4
i "CheckBounds" 0 .02  2    2.9  6.9  4 2
i "CheckBounds" 0 .02  2    7    1e20 7 2
i "CheckBounds" 0 .02  2    8    0    8 1
; Empty, reversed and entirely out-of-range searches return silence.
i "CheckBounds" 0 .02  2    3    3    0 0
i "CheckBounds" 0 .02  2    6    2    0 0
i "CheckBounds" 0 .02  2    1e20 0    0 0
i "CheckResults" .03 .01
e
</CsScore>
</CsoundSynthesizer>
