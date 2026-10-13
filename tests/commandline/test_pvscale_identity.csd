<CsTest>
description = "pvscale preserves every bin at unit pitch ratio and applies gain to the whole spectrum"

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
 ; A different amplitude in each bin exposes mismatched envelope indices.
 ; Include DC and Nyquist so endpoint values must also survive the copy.
 kInput[] init 130
 kBin = 0
 while kBin <= 64 do
  kInput[2*kBin] = (1+kBin%5)/8
  kInput[2*kBin+1] = kBin*62.5
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 fOutput pvscale fInput, 1, p4, p5, p6
 kOutput[] init 130
 kFrame pvs2array kOutput, fOutput
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  kBin = 0
  while kBin <= 64 do
   kExpectedAmp = kInput[2*kBin]*p5
   kExpectedFreq = kInput[2*kBin+1]
   if !(abs(kOutput[2*kBin]-kExpectedAmp) < .000001 && abs(kOutput[2*kBin+1]-kExpectedFreq) < .001) then
    printks "formants %g, gain %g, bin %g: amplitude %g expected %g; frequency %g expected %g\n", 0, p4, p5, kBin, kOutput[2*kBin], kExpectedAmp, kOutput[2*kBin+1], kExpectedFreq
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 10 then
  prints "pvscale identity checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Formant mode, gain, cepstrum coefficients.
; A pitch ratio of one must preserve the spectrum in every formant mode.
i 1 0 .01 0 1 20
i 1 0 .01 1 1 20
i 1 0 .01 2 1 20
i 1 0 .01 3 1 20
; Gain also scales DC and Nyquist.
i 1 0 .01 0 0 20
i 1 0 .01 1 .5 20
i 1 0 .01 2 2 20
i 1 0 .01 3 0 20
; Zero uses the default count; a large count keeps all coefficients.
i 1 0 .01 1 1 0
i 1 0 .01 2 1 1e20
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
