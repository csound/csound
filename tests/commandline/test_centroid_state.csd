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

; Changing FFT size must produce the same window as a fresh instance.
instr 1
 kCycle init 0
 kCycle += 1
 aFirst oscili .5, 512
 aSecond oscili .2, 1536
 aInput = aFirst+aSecond+.1
 if kCycle == 65 then
  reinit ANALYZE
 endif
ANALYZE:
 iSize = (i(kCycle) >= 65 ? p5 : p4)
 kActual centroid aInput, 1, iSize
 rireturn
 kReference centroid aInput, 1, p5
 if kCycle >= 128 && !(abs(kActual-kReference) < .01) then
  printks "centroid reset %g->%g: %g versus %g\n", 0, p4, p5, kActual, kReference
  exitnowk(-1)
 endif
 if kCycle == 160 then
  gkChecks += 1
 endif
endin

; Exact FFT-bin tones must not acquire a half-bin frequency offset.
; DC and Nyquist each have one neighboring Hann-window side bin.
instr 2
 kCycle init 0
 kCycle += 1
 aTone oscili .5, 512
 aDC = 1
 aPhase phasor sr/2
 aNyquist = cos(aPhase*6.283185307179586)
 kTone centroid aTone, 1, 64
 kDC centroid aDC, 1, 64
 kNyquist centroid aNyquist, 1, 64
 kTrigger = (kCycle <= 16 ? 1 : 0)
 kHeld centroid aTone, kTrigger, 64
 if kCycle >= 16 then
  if !(abs(kTone-512)+abs(kDC-128/3)+abs(kNyquist-(4096-128/3))+abs(kHeld-512) < .02) then
   printks "centroid bin frequencies: tone=%g DC=%g Nyquist=%g held=%g\n", 0, kTone, kDC, kNyquist, kHeld
   exitnowk(-1)
  endif
 endif
 if kCycle == 160 then
  gkChecks += 1
 endif
endin

instr 3
 iDC[] fillarray 1, 0, 0, 0, 0
 iNyquist[] fillarray 0, 0, 0, 0, 1
 iMixed[] fillarray 1, 2, 0, 0, 1
 iSilence[] fillarray 0, 0
 iD centroid iDC
 iN centroid iNyquist
 iM centroid iMixed
 iZ centroid iSilence
 if iD != 0 || iN != 4096 || iM != 1536 || iZ != 0 then
  prints "centroid magnitude-bin mismatch: %g %g %g %g\n", iD, iN, iM, iZ
  exitnow(-1)
 endif
 kBins[] fillarray 0, 0, 0, 0, 0
 kCycle init 0
 kCycle += 1
 kBins[0] = 1
 kBins[4] = kCycle
 kActual centroid kBins
 kExpected = 4096*kCycle/(1+kCycle)
 if !(abs(kActual-kExpected) < .001) then
  printks "centroid changing magnitude bins: %g versus %g\n", 0, kActual, kExpected
  exitnowk(-1)
 endif
 if kCycle == 160 then
  gkChecks += 1
 endif
endin
instr 99
 if i(gkChecks) != 6 then
  prints "centroid checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .3125 256 64
i 1 0 .3125 64 256
i 1 0 .3125 128 128
i 1 0 .3125 256 63
i 2 0 .3125
i 3 0 .3125
i 99 .32 .01
e
</CsScore>
</CsoundSynthesizer>
