<CsTest>
description = "pvsenvftw bounds coefficient counts and preserves the full envelope when all coefficients are kept"

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
 iBins = p6
 iEnvelope ftgen 0, 0, -iBins, 2, 0
 kInput[] init 2*iBins+2
 kBin = 0
 while kBin <= iBins do
  kInput[2*kBin] = .125*2^(kBin%3)
  kInput[2*kBin+1] = kBin*sr/(2*iBins)
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 kUpdated pvsenvftw fInput, iEnvelope, p4, 1, p5
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  ; With every coefficient kept, the log/exp round trip preserves each bin.
  kBin = 0
  while kBin < iBins do
   kActual table kBin, iEnvelope
   if !(abs(kActual-kInput[2*kBin]) < .000001) then
    printks "method %g, coefficients %g, bin %g: envelope %g expected %g\n", 0, p4, p5, kBin, kActual, kInput[2*kBin]
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 2
 ; Phase spectra still contain usable amplitudes. A silent one is valid.
 iEnvelope ftgen 0, 0, 64, 2, 0
 fInput pvsinit 128, 32, 128, 1, 1
 kUpdated pvsenvftw fInput, iEnvelope
 kBin = 0
 while kBin < 64 do
  kActual table kBin, iEnvelope
  if !(abs(kActual) < .000001) then
   prints "pvsenvftw changed silent phase-spectrum amplitudes\n"
   exitnowk(-1)
  endif
  kBin += 1
 od
 kOnce init 1
 if kOnce == 1 then
  gkChecks += 1
  kOnce = 0
 endif
endin

instr 99
 if i(gkChecks) != 9 then
  prints "pvsenvftw coefficient checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Method, coefficient count, number of envelope bins.
i 1 0 .0125 1 128 128
i 1 0 .0125 2 128 128
i 1 0 .0125 1 1e30 128
i 1 0 .0125 2 1e30 128
; Nonpositive counts use 80, bounded by the size of this small spectrum.
i 1 0 .0125 1 0 4
i 1 0 .0125 2 0 4
i 1 0 .0125 1 -1 4
i 1 0 .0125 2 -1 4
i 2 0 .0125
i 99 .02 .00125
e
</CsScore>
</CsoundSynthesizer>
