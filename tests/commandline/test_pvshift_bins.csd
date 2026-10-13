<CsTest>
description = "pvshift maps positive and negative shifts to the cutoff bin and preserves unshifted bins"

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
 kInput[] init 130
 kBin = 0
 while kBin <= 64 do
  kInput[2*kBin] = (1+kBin%5)/8
  kInput[2*kBin+1] = kBin*62.5
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 ; Keeping all cepstrum coefficients makes the envelope equal the input
 ; amplitudes. In formant modes, a moved bin takes the destination's amplitude.
 fOutput pvshift fInput, p4*62.5, p5*62.5, p6, 1, 64
 kOutput[] init 130
 kFrame pvs2array kOutput, fOutput
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kBin = 0
  while kBin <= 64 do
   kExpectedAmp = kInput[2*kBin]
   kExpectedFreq = kInput[2*kBin+1]
   if kBin >= p5 && kBin > 0 && kBin < 64 then
    kSourceBin = kBin-p4
    if kSourceBin >= p5 && kSourceBin > 0 && kSourceBin < 64 then
     kExpectedAmp = p6 == 0 ? kInput[2*kSourceBin] : kInput[2*kBin]
    else
     kExpectedAmp = 0
    endif
   endif
   ; An empty bin has no meaningful frequency.
   if !(abs(kOutput[2*kBin]-kExpectedAmp) < .000001) || (kExpectedAmp != 0 && !(abs(kOutput[2*kBin+1]-kExpectedFreq) < .001)) then
    printks "shift %g bins, cutoff bin %g, formants %g, bin %g: amplitude %g expected %g; frequency %g expected %g\n", 0, p4, p5, p6, kBin, kOutput[2*kBin], kExpectedAmp, kOutput[2*kBin+1], kExpectedFreq
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 9 then
  prints "pvshift bin mapping checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Shift in bins, cutoff bin, formant mode.
i 1 0 .01 1 16 0
i 1 0 .01 -1 16 0
i 1 0 .01 1 16 1
i 1 0 .01 -1 16 1
i 1 0 .01 1 16 2
i 1 0 .01 -1 16 2
; Shifts beyond the available bins discard only the shifted part.
i 1 0 .01 100 16 0
i 1 0 .01 -100 16 0
; A cutoff above Nyquist must not wrap back into the middle of the spectrum.
i 1 0 .01 1 80 0
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
