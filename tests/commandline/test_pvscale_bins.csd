<CsTest>
description = "pvscale preserves formants when transposing up or down and discards bins outside the spectrum"

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
  kInput[2*kBin] = p6 == 1 ? .25 : (1+kBin%5)/8
  kInput[2*kBin+1] = kBin*62.5
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 ; Keeping all cepstrum coefficients makes the envelope equal the input
 ; amplitudes. Formant modes then use the destination bin's amplitude.
 fOutput pvscale fInput, p4, p5, 1, 64
 kOutput[] init 130
 kFrame pvs2array kOutput, fOutput
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kExpected[] init 130
  ; DC and Nyquist pass through unchanged. Other bins start empty.
  kExpected[0] = kInput[0]
  kExpected[128] = kInput[128]
  kExpected[129] = kInput[129]
  kSource = 1
  while kSource < 64 do
   kDestination = int(kSource*p4+.5)
   if kDestination > 0 && kDestination < 64 then
    ; If sources share a destination, pvscale keeps the last source bin.
    kExpected[2*kDestination] = p5 == 0 ? kInput[2*kSource] : kInput[2*kDestination]
    kExpected[2*kDestination+1] = kInput[2*kSource+1]*p4
   endif
   kSource += 1
  od
  kBin = 0
  while kBin <= 64 do
   kAmplitudeError = abs(kOutput[2*kBin]-kExpected[2*kBin])
   kFrequencyError = abs(kOutput[2*kBin+1]-kExpected[2*kBin+1])
   ; Empty bins have no meaningful frequency.
   if !(kAmplitudeError < .000001) || (kExpected[2*kBin] != 0 && !(kFrequencyError < .001)) then
    printks "ratio %g, formants %g, bin %g: amplitude %g expected %g; frequency %g expected %g\n", 0, p4, p5, kBin, kOutput[2*kBin], kExpected[2*kBin], kOutput[2*kBin+1], kExpected[2*kBin+1]
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 10 then
  prints "pvscale bin mapping checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Pitch ratio, formant mode, use a flat spectrum.
i 1 0 .01 .5 0 0
i 1 0 .01 2 0 0
i 1 0 .01 .5 1 0
i 1 0 .01 2 1 0
i 1 0 .01 .5 2 0
i 1 0 .01 2 2 0
; No interior bin survives these ratios.
i 1 0 .01 0 0 0
i 1 0 .01 100 0 0
; A flat spectrum also stays flat under the experimental moving average.
i 1 0 .01 .5 3 1
i 1 0 .01 2 3 1
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
