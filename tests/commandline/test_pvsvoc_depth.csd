<CsTest>
description = "pvsvoc dry and wet amplitudes include DC and Nyquist"

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
 iAmplitude = p4
 iExcitation = p5
 iDepth = p6
 iGain = p7
 iNyquistOnly = p8
 kAmplitude[] init 130
 kExcitation[] init 130
 kBin = 0
 while kBin < 65 do
  kLevel = iNyquistOnly == 0 || kBin == 64 ? 1 : 0
  kAmplitude[2*kBin] = iAmplitude*kLevel
  kAmplitude[2*kBin+1] = kBin*sr/128
  kExcitation[2*kBin] = iExcitation*kLevel
  kExcitation[2*kBin+1] = kBin*sr/128*.75
  kBin += 1
 od
 fAmplitude pvsfromarray kAmplitude, 32
 fExcitation pvsfromarray kExcitation, 32
 fOutput pvsvoc fAmplitude, fExcitation, iDepth, iGain, 20
 kOutput[] init 130
 kFrame pvs2array kOutput, fOutput

 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  ; A flat excitation has no spectral shape to remove. Its level remains
  ; as a multiplier of the amplitude envelope in the fully wet result.
  iWet = iAmplitude*iExcitation
  iMixed = (iAmplitude*(1-iDepth) + iWet*iDepth)*iGain
  kBin = 0
  while kBin < 65 do
   kExpectedAmplitude = iNyquistOnly == 0 || kBin == 64 ? iMixed : 0
   kExpectedFrequency = kBin*sr/128*(1-iDepth*.25)
   if !(abs(kOutput[2*kBin]-kExpectedAmplitude) < .000001 && abs(kOutput[2*kBin+1]-kExpectedFrequency) < .001) then
    printks "depth %g, Nyquist only %g, bin %g: amplitude=%g expected=%g; frequency=%g expected=%g\n", 0, iDepth, iNyquistOnly, kBin, kOutput[2*kBin], kExpectedAmplitude, kOutput[2*kBin+1], kExpectedFrequency
    exitnowk(-1)
   endif
   kBin += 1
  od
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 9 then
  prints "pvsvoc depth checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Amplitude input, excitation level, depth, output gain, Nyquist-only flag.
i 1 0 .01 .25 1   0   1 0
i 1 0 .01 .25 1   1   1 0
i 1 0 .01 .25 .5  .5  2 0
; A silent amplitude input stays silent. A silent excitation contributes
; nothing to the wet signal but must not remove the dry signal.
i 1 0 .01 0   1   .5  1 0
i 1 0 .01 .25 0   0   1 0
i 1 0 .01 .25 0   1   1 0
i 1 0 .01 .25 0   .5  1 0
; Nyquist must work even when every other bin is silent.
i 1 0 .01 .25 1   0   1 1
i 1 0 .01 .25 .5  1   2 1
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
