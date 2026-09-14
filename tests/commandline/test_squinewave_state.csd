<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode OneSample, aa, aaaai
 setksmps 1
 aFreq, aClip, aSkew, aSync, iPhase xin
 aOut, aPulse squinewave aFreq, aClip, aSkew, aSync, 8, iPhase
 xout aOut, aPulse
endop

instr 1
 aFreq = p4
 aClip = p5
 aSkew = p6
 aSync mpulse p8, .01
 aExpected, aExpectedPulse OneSample aFreq, aClip, aSkew, aSync, p7
 aActual, aPulse squinewave aFreq, aClip, aSkew, aSync, 8, p7
 kBlock init 0
 kBlock += 1
 kN = 0
 while kN < ksmps do
  kActual vaget kN, aActual
  kExpected vaget kN, aExpected
  kPulse vaget kN, aPulse
  kExpectedPulse vaget kN, aExpectedPulse
  if !(abs(kActual-kExpected) < .000003) || kPulse != kExpectedPulse then
   printks "squinewave start=%g freq=%g phase=%g block=%g sample=%g actual=%g expected=%g sync=%g expectedSync=%g\n", 0, p2, p4, p7, kBlock, kN, kActual, kExpected, kPulse, kExpectedPulse
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 8 then
  gkChecks += 1
 endif
endin

instr 2
 setksmps 1
 kCount init 0
 kCount += 1
 aFreq = (p4 == 0 && kCount < 20 ? -100 : 100)
 aClip = .7
 aSkew = .3
 aSync = (p4 != 0 && kCount == 19 ? 1 : 0)
 ; Negative iphase preserves history, including a pending sync sweep.
 if p4 == 2 then
  aExpected, aExpectedPulse squinewave aFreq, aClip, aSkew, aSync, 8, -1
 endif
 if kCount == 20 then
  reinit RESET
 endif
RESET:
 aActual, aPulse squinewave aFreq, aClip, aSkew, aSync, 8, p5
 rireturn
 if kCount >= 20 then
  if p4 != 2 then
   ; This instance starts performing at the same sample as the phase reset.
   aExpected, aExpectedPulse squinewave aFreq, aClip, aSkew, aSync, 8, p5
  endif
  kActual downsamp aActual
  kExpected downsamp aExpected
  kPulse downsamp aPulse
  kExpectedPulse downsamp aExpectedPulse
  if !(abs(kActual-kExpected) < .000003) || kPulse != kExpectedPulse then
   printks "squinewave reinit case=%g sample=%g actual=%g expected=%g sync=%g expectedSync=%g\n", 0, p4, kCount, kActual, kExpected, kPulse, kExpectedPulse
   exitnowk(-1)
  endif
 endif
 if kCount == 80 then
  gkChecks += 1
 endif
endin

instr 3
 setksmps 1
 kCount init 0
 kHold init 0
 kCount += 1
 aFreq = (kCount <= 16 || (kCount >= 40 && kCount < 56) ? 0 : 100)
 aClip = p4
 aSkew = p5
 aOut squinewave aFreq, aClip, aSkew, 0, 8, p6
 kActual downsamp aOut
 if kCount <= 16 then
  kExpected = cos(2*$M_PI*p6)
 elseif kCount == 40 then
  kHold = kActual
  kExpected = kHold
 elseif kCount > 40 && kCount < 56 then
  kExpected = kHold
 else
  kExpected = kActual
 endif
 if !(abs(kActual-kExpected) < .000003) then
  printks "squinewave stopped clip=%g skew=%g sample=%g actual=%g expected=%g\n", 0, p4, p5, kCount, kActual, kExpected
  exitnowk(-1)
 endif
 if kCount == 80 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 16 then
  prints "squinewave checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Aligned and partial blocks, sine and shaped waves, and hard sync.
i 1 0 .025 1000 0 0 0 0
i 1 0 .025 100 .7 .3 .75 0
i 1 0 .025 100 .7 .3 1.75 1
i 1 .030625 .02525 1000 0 0 0 0
i 1 .030625 .02525 100 .7 .3 .75 0
i 1 .030625 .02525 100 .7 -.3 1.75 0
i 1 .030625 .02525 100 1 .9 .25 0
i 1 .030625 .02525 -100 .7 .3 .25 0
i 1 .030625 .02525 100 .7 .3 1.75 1
i 1 .030625 .02525 100 .7 .3 -1 0
; Explicit phase reset and documented phase preservation.
i 2 .06 .0125 0 .25
i 2 .06 .0125 1 .25
i 2 .06 .0125 2 -1
i 3 .06 .0125 1 0 0
i 3 .06 .0125 0 1 0
i 3 .06 .0125 1 -1 .25
i 99 .08 .001
e
</CsScore>
</CsoundSynthesizer>
