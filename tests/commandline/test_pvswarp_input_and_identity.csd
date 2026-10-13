<CsTest>
description = "pvswarp leaves its input unchanged and preserves every bin without warping"

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
  if p7 == 1 then
   kInput[2*kBin] = 0
  elseif p7 == 2 then
   kInput[2*kBin] = kBin == 64 ? .25 : 0
  endif
  kInput[2*kBin+1] = kBin*62.5
  kBin += 1
 od
 fInput pvsfromarray kInput, 32
 kBefore[] init 130
 kBeforeFrame pvs2array kBefore, fInput
 ; Distinct input and output variables: consumers of fInput must agree
 ; whether they run before or after pvswarp.
 fOutput pvswarp fInput, 1, 0, 0, p4, p5, p6
 kAfter[] init 130
 kAfterFrame pvs2array kAfter, fInput
 kOutput[] init 130
 kOutputFrame pvs2array kOutput, fOutput

 kBin = 0
 while kBin <= 64 do
  if kBefore[2*kBin] != kAfter[2*kBin] || kBefore[2*kBin+1] != kAfter[2*kBin+1] then
   printks "method %g changed input bin %g: amplitude %g to %g, frequency %g to %g\n", 0, p4, kBin, kBefore[2*kBin], kAfter[2*kBin], kBefore[2*kBin+1], kAfter[2*kBin+1]
   exitnowk(-1)
  endif
  ; With ratio 1 and shift 0, only the requested gain changes amplitudes.
  kExpectedAmp = kBefore[2*kBin]*p5
  kAmpError = abs(kOutput[2*kBin]-kExpectedAmp)
  kFreqError = abs(kOutput[2*kBin+1]-kBefore[2*kBin+1])
  if !(kAmpError < .000001 && kFreqError < .001) then
   printks "method %g, gain %g, bin %g: amplitude %g expected %g; frequency %g expected %g\n", 0, p4, p5, kBin, kOutput[2*kBin], kExpectedAmp, kOutput[2*kBin+1], kBefore[2*kBin+1]
   exitnowk(-1)
  endif
  kBin += 1
 od
 kCycle init 0
 kCycle += 1
 if kCycle == 8 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 12 then
  prints "pvswarp input and identity checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Method, gain, cepstrum coefficients, spectrum (0: colored, 1: silent, 2: Nyquist only).
i 1 0 .01 1 1 20 0
i 1 0 .01 2 1 20 0
i 1 0 .01 3 1 20 0
; Gain must also affect DC and Nyquist.
i 1 0 .01 1 0 20 0
i 1 0 .01 2 .5 20 0
i 1 0 .01 3 2 20 0
; Default and oversized coefficient counts.
i 1 0 .01 1 1 0 0
i 1 0 .01 2 1 1e20 0
; Silence and energy confined to the endpoint.
i 1 0 .01 1 1 20 1
i 1 0 .01 2 1 20 1
i 1 0 .01 1 1 20 2
i 1 0 .01 2 1 20 2
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
