<CsTest>
description = "Sliding pvsynth reconstructs DC, interior bins and Nyquist with the correct sign"

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

instr CheckReconstruction
 iSize = p4
 iPi = 3.141592653589793
 ; Sliding synthesis returns the middle sample of the analysis window.
 ; For these windows its gain is one and its delay is N/2-1 samples.
 iDelay = iSize/2-1
 iNyquistSign = (iDelay % 2 == 0 ? 1 : -1)
 iToneShift = (iDelay % 8)/8
 aNyquistPhase phasor sr/2
 aNyquist = .25*(1-4*aNyquistPhase)
 aTonePhase phasor sr/8
 aTone = .25*sin(2*iPi*aTonePhase)
 aDelayedTone = .25*sin(2*iPi*(aTonePhase-iToneShift))
 if p5 == 0 then
  aInput = .25
  aExpected = .25
 elseif p5 == 1 then
  aInput = aNyquist
  aExpected = iNyquistSign*aNyquist
 elseif p5 == 2 then
  aInput = aTone
  aExpected = aDelayedTone
 else
  aInput = .125 + aNyquist + aTone
  aExpected = .125 + iNyquistSign*aNyquist + aDelayedTone
 endif
 aInput *= p7
 aExpected *= p7
 fAnalysis pvsanal aInput, iSize, 1, iSize, p6
 aOutput pvsynth fAnalysis
 kCycle init 0
 kCycle += 1
 ; Wait for the window to fill, then compare signed samples, not just RMS.
 if kCycle == 32 then
  kIndex = 0
  while kIndex < ksmps do
   kActual vaget kIndex, aOutput
   kExpected vaget kIndex, aExpected
   if !(abs(kActual-kExpected) < .0005) then
    printks "pvsynth size=%g signal=%g window=%g gain=%g sample=%g: got %g, expected %g\n", 0, iSize, p5, p6, p7, kIndex, kActual, kExpected
    exitnowk(-1)
   endif
   kIndex += 1
  od
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 18 then
  prints "pvsynth reconstruction checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Size, signal (DC=0, Nyquist=1, interior=2, mixture=3), window, gain.
; N/2 is even for size 64 and odd for size 66.
i "CheckReconstruction" 0 .1 64 0 1 1
i "CheckReconstruction" 0 .1 64 1 1 1
i "CheckReconstruction" 0 .1 64 2 1 1
i "CheckReconstruction" 0 .1 64 3 1 1
i "CheckReconstruction" 0 .1 66 0 1 1
i "CheckReconstruction" 0 .1 66 1 1 1
i "CheckReconstruction" 0 .1 66 2 1 1
i "CheckReconstruction" 0 .1 66 3 1 1
; Small transforms, a larger non-power-of-two size, and reversed polarity.
i "CheckReconstruction" 0 .1 2 3 1 1
i "CheckReconstruction" 0 .1 4 3 1 1
i "CheckReconstruction" 0 .1 6 3 1 1
i "CheckReconstruction" 0 .1 96 3 1 1
i "CheckReconstruction" 0 .1 64 0 1 -1
i "CheckReconstruction" 0 .1 64 1 1 -1
; Hamming and rectangular windows also need the correct Nyquist sign.
i "CheckReconstruction" 0 .1 64 1 0 1
i "CheckReconstruction" 0 .1 66 1 0 1
i "CheckReconstruction" 0 .1 64 1 9 1
i "CheckReconstruction" 0 .1 66 1 9 1
i "CheckResults" .11 .01
e
</CsScore>
</CsoundSynthesizer>
