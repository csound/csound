<CsTest>
description = "pvsenvftw keeps flat envelopes flat and preserves both edges of its moving average"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 6400
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 iMethod = p4
 iAmplitude = p5
 iBins = p6
 iGain = p7
 iEnvelope ftgen 0, 0, -iBins, 2, 0
 kInput[] init 2*iBins+2
 kBin = 0
 while kBin <= iBins do
  kInput[2*kBin] = iAmplitude
  kInput[2*kBin+1] = kBin*sr/(2*iBins)
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 kUpdated pvsenvftw fInput, iEnvelope, iMethod, iGain
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  ; Every smoothing method must preserve a constant, including silence.
  kBin = 0
  while kBin < iBins do
   kActual table kBin, iEnvelope
   if !(abs(kActual-iAmplitude*iGain) < .000001) then
    printks "method %g, %g bins, bin %g: envelope %g expected %g\n", 0, iMethod, iBins, kBin, kActual, iAmplitude*iGain
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 2
 iEnvelope ftgen 0, 0, 64, 2, 0
 kInput[] init 130
 kCycle init 0
 kCycle += 1
 kBin = 0
 while kBin <= 64 do
  kInput[2*kBin] = .125*2^(kBin%3)
  kInput[2*kBin+1] = kBin*50
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 ; Switch to cepstral smoothing and back. Method 3 must not retain its
 ; previous work buffer, whose entries have a different layout in method 1.
 kMethod = kCycle <= 8 || kCycle > 16 ? 3 : 1
 kUpdated pvsenvftw fInput, iEnvelope, kMethod, 1, 64
 if kCycle == 8 || kCycle == 16 || kCycle == 24 then
  kBin = 0
  while kBin < 64 do
   kExpected = kInput[2*kBin]
   if kMethod == 3 && kBin >= 5 && kBin < 59 then
    ; A mean in log space equals the geometric mean of these ten amplitudes.
    kProduct = 1
    kNeighbor = kBin-5
    while kNeighbor < kBin+5 do
     kProduct *= kInput[2*kNeighbor]
     kNeighbor += 1
    od
    kExpected = kProduct^.1
   endif
   kActual table kBin, iEnvelope
   if !(abs(kActual-kExpected) < .000001) then
    printks "method %g, bin %g: envelope %g expected %g\n", 0, kMethod, kBin, kActual, kExpected
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 11 then
  prints "pvsenvftw envelope checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Method, constant amplitude, number of envelope bins, gain.
i 1 0 .0125 1 .25 64 1
i 1 0 .0125 2 .25 64 1
i 1 0 .0125 3 .25 64 1
i 1 0 .0125 1 0 64 1
i 1 0 .0125 2 0 64 1
i 1 0 .0125 3 0 64 1
; Small frames have no room for the moving window: preserve every entry.
i 1 0 .0125 3 .25 4 2
i 1 0 .0125 3 .25 8 .5
i 2 .02 .035
i 99 .06 .00125
e
</CsScore>
</CsoundSynthesizer>
