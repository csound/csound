<CsTest>
description = "Sliding pvsanal windows agree with a direct windowed DFT"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckWindow
 iSize = p5
 iPi = 3.141592653589793
 ; Periodic windows have the form a0 - a1*cos(x) + a2*cos(2*x).
 iA0 = p6
 iA1 = p7
 iA2 = p8
 ; Include DC, Nyquist, both edge bins, and an interior bin. This exercises
 ; the mirrored terms at each boundary as well as the ordinary window sum.
 aPhase phasor sr/iSize
 aAngle = 2*iPi*aPhase
 aInput = .125 + .25*sin(aAngle) + .2*cos((iSize/2-1)*aAngle) + .1*cos(iSize/2*aAngle) + .15*sin(7*aAngle)
 fAnalysis pvsanal aInput, iSize, 1, iSize, p4
 ; Save the actual input samples for an independent time-domain DFT.
 ; One extra sample also gives us the preceding window for frequency checks.
 kHistory[] init iSize+1
 kPosition init 0
 kSampleIndex = 0
 while kSampleIndex < ksmps do
  kSample vaget kSampleIndex, aInput
  kHistory[kPosition] = kSample
  kPosition = (kPosition+1) % (iSize+1)
  kSampleIndex += 1
 od
 kCycle init 0
 kCycle += 1
 if kCycle == 16 || kCycle == 17 then
  ; Compare the spectrum at the last sample of this block.
  kBin = 0
  while kBin <= iSize/2 do
   aMagnitudes, aFrequencies pvsbin fAnalysis, kBin
   kMagnitude vaget ksmps-1, aMagnitudes
   kFrequency vaget ksmps-1, aFrequencies
   kReal = 0
   kImag = 0
   kPreviousReal = 0
   kPreviousImag = 0
   kIndex = 0
   while kIndex < iSize do
    kAngle = 2*iPi*kIndex/iSize
    kWeight = iA0 - iA1*cos(kAngle) + iA2*cos(2*kAngle)
    kSample = kHistory[(kPosition+1+kIndex) % (iSize+1)]
    kPreviousSample = kHistory[(kPosition+kIndex) % (iSize+1)]
    ; Reduce the angle first to limit rounding in the float build.
    kBinAngle = 2*iPi*((kBin*kIndex) % iSize)/iSize
    kReal += kSample*kWeight*cos(kBinAngle)
    kImag -= kSample*kWeight*sin(kBinAngle)
    kPreviousReal += kPreviousSample*kWeight*cos(kBinAngle)
    kPreviousImag -= kPreviousSample*kWeight*sin(kBinAngle)
    kIndex += 1
   od
   kExpectedMagnitude = sqrt(kReal*kReal+kImag*kImag)
   if !(abs(kMagnitude-kExpectedMagnitude) < .0001*(1+kExpectedMagnitude)) then
    printks "pvsanal window=%g size=%g bin=%g: magnitude %g, expected %g\n", 0, p4, iSize, kBin, kMagnitude, kExpectedMagnitude
    exitnowk(-1)
   endif
   ; Phase differences are meaningful only when both frames have energy.
   if kExpectedMagnitude > .01 && kPreviousReal*kPreviousReal+kPreviousImag*kPreviousImag > .0001 then
    kDifference = taninv2(kImag, kReal) - taninv2(kPreviousImag, kPreviousReal) - 2*iPi*kBin/iSize
    kDifference -= 2*iPi*ceil((kDifference-iPi)/(2*iPi))
    kExpectedFrequency = sr*(kBin/iSize+kDifference/(2*iPi))
    ; At the phase wrap boundary, either result can differ by one full
    ; sample rate. Those frequencies have the same sample-to-sample phase.
    kFrequencyError = abs(kFrequency-kExpectedFrequency)
    kFrequencyError = min(kFrequencyError, abs(kFrequencyError-sr))
    if !(kFrequencyError < .1) then
     printks "pvsanal window=%g size=%g bin=%g: frequency %g, expected %g\n", 0, p4, iSize, kBin, kFrequency, kExpectedFrequency
     exitnowk(-1)
    endif
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 80 then
  prints "pvsanal window checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Window, size, and the three coefficients of its time-domain formula.
; Hamming, Hann and rectangular windows also check the reference DFT.
i "CheckWindow" 0 .04 0 64 .54 .46 0
i "CheckWindow" 0 .04 1 64 .5 .5 0
i "CheckWindow" 0 .04 9 64 1 0 0
i "CheckWindow" 0 .04 4 64 .42 .5 .08
i "CheckWindow" 0 .04 5 64 .42659071367153912296 .49656061908856405847 .076848667239896818573
i "CheckWindow" 0 .04 6 64 .375 .5 .125
i "CheckWindow" 0 .04 7 64 .44959 .49364 .05677
i "CheckWindow" 0 .04 8 64 .42323 .4973406 .0782793
; Sliding analysis also supports sizes that are not powers of two.
i "CheckWindow" 0 .04 0 96 .54 .46 0
i "CheckWindow" 0 .04 1 96 .5 .5 0
i "CheckWindow" 0 .04 9 96 1 0 0
i "CheckWindow" 0 .04 4 96 .42 .5 .08
i "CheckWindow" 0 .04 5 96 .42659071367153912296 .49656061908856405847 .076848667239896818573
i "CheckWindow" 0 .04 6 96 .375 .5 .125
i "CheckWindow" 0 .04 7 96 .44959 .49364 .05677
i "CheckWindow" 0 .04 8 96 .42323 .4973406 .0782793
; Small transforms must also mirror and wrap the neighbouring bins.
i "CheckWindow" 0 .04 0 2 .54 .46 0
i "CheckWindow" 0 .04 1 2 .5 .5 0
i "CheckWindow" 0 .04 9 2 1 0 0
i "CheckWindow" 0 .04 4 2 .42 .5 .08
i "CheckWindow" 0 .04 5 2 .42659071367153912296 .49656061908856405847 .076848667239896818573
i "CheckWindow" 0 .04 6 2 .375 .5 .125
i "CheckWindow" 0 .04 7 2 .44959 .49364 .05677
i "CheckWindow" 0 .04 8 2 .42323 .4973406 .0782793
i "CheckWindow" 0 .04 0 4 .54 .46 0
i "CheckWindow" 0 .04 1 4 .5 .5 0
i "CheckWindow" 0 .04 9 4 1 0 0
i "CheckWindow" 0 .04 4 4 .42 .5 .08
i "CheckWindow" 0 .04 5 4 .42659071367153912296 .49656061908856405847 .076848667239896818573
i "CheckWindow" 0 .04 6 4 .375 .5 .125
i "CheckWindow" 0 .04 7 4 .44959 .49364 .05677
i "CheckWindow" 0 .04 8 4 .42323 .4973406 .0782793
i "CheckWindow" 0 .04 0 6 .54 .46 0
i "CheckWindow" 0 .04 1 6 .5 .5 0
i "CheckWindow" 0 .04 9 6 1 0 0
i "CheckWindow" 0 .04 4 6 .42 .5 .08
i "CheckWindow" 0 .04 5 6 .42659071367153912296 .49656061908856405847 .076848667239896818573
i "CheckWindow" 0 .04 6 6 .375 .5 .125
i "CheckWindow" 0 .04 7 6 .44959 .49364 .05677
i "CheckWindow" 0 .04 8 6 .42323 .4973406 .0782793
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
