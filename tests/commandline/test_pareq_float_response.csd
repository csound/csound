<CsTest>
description = "pareq keeps its response and history with float coefficients"
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

; The requested gain applies at DC for a low shelf, at Nyquist for a
; high shelf, and at the center frequency for a peaking filter.
instr 1
  if p4 == 1 then
    aInput = 1
  elseif p4 == 2 then
    aInput oscili 1, sr/2, -1, .25
  else
    aInput oscili 1, p5
  endif
  aOutput pareq aInput, p5, p6, p7, p4
  if timeinsts() >= 10 then
    kN = 0
    while kN < ksmps do
      kInput vaget kN, aInput
      kOutput vaget kN, aOutput
      if !(abs(kOutput-p6*kInput) < .0003*(1+p6)) then
        printks "FAIL pareq mode=%g frequency=%g gain=%g Q=%g output=%g expected=%g\n", 0, p4, p5, p6, p7, kOutput, p6*kInput
        exitnowk -1
      endif
      kN += 1
    od
    gkChecks += 1
    turnoff
  endif
endin

; Compare changing parameters with the direct recurrence at frequencies
; where its coefficients are well separated. Include the reflection switch.
instr 2
  setksmps 1
  kSample init 0
  kX1 init 0
  kX2 init 0
  kY1 init 0
  kY2 init 0
  kFrequency = 1024+(kSample % 5)*512
  kGain = (kSample % 3 == 0 ? .25 : (kSample % 3 == 1 ? 1 : 4))
  kQ = .5+(kSample % 4)*.25
  kInput = (kSample % 11-5)/8
  aInput = kInput
  aOutput pareq aInput, kFrequency, kGain, kQ, p4
  aReuse = aInput
  aReuse pareq aReuse, kFrequency, kGain, kQ, p4
  kK = tan($M_PI*(p4 == 2 ? sr/2-kFrequency : kFrequency)/sr)
  kKK = kK*kK
  kNorm = 1/(1+kK/kQ+kKK)
  kSign = (p4 == 2 ? -1 : 1)
  kA1 = 2*kSign*(kKK-1)*kNorm
  kA2 = (1-kK/kQ+kKK)*kNorm
  kBand = (p4 == 0 ? kGain*kK/kQ : sqrt(2*kGain)*kK)
  kLow = (p4 == 0 ? kKK : kGain*kKK)
  kB0 = (1+kBand+kLow)*kNorm
  kB1 = 2*kSign*(kLow-1)*kNorm
  kB2 = (1-kBand+kLow)*kNorm
  kExpected = kB0*kInput+kB1*kX1+kB2*kX2-kA1*kY1-kA2*kY2
  kActual downsamp aOutput
  kReuse downsamp aReuse
  if !(abs(kActual-kExpected) < .0001*(1+abs(kExpected)) && kActual == kReuse) then
    printks "FAIL pareq changing parameters mode=%g sample=%g output=%g reuse=%g expected=%g\n", 0, p4, kSample, kActual, kReuse, kExpected
    exitnowk -1
  endif
  kX2 = kX1
  kX1 = kInput
  kY2 = kY1
  kY1 = kExpected
  kSample += 1
  if kSample == 96 then
    gkChecks += 1
    turnoff
  endif
endin

; Large finite inputs must not overflow an unscaled input sum.
instr 3
  aInput = 1
  if p4 == 0 then
    aInput oscili 1, 3072
  endif
  aLarge = aInput*1e38
  aSmallOut pareq aInput, 1, p5, .70710678, p4
  aLargeOut pareq aLarge, 1, p5, .70710678, p4
  kN = 0
  while kN < ksmps do
    kSmall vaget kN, aSmallOut
    kLarge vaget kN, aLargeOut
    if !(abs(kLarge/1e38-kSmall) < .0001) then
      printks "FAIL pareq input scaling mode=%g gain=%g small=%g large=%g\n", 0, p4, p5, kSmall, kLarge
      exitnowk -1
    endif
    kN += 1
  od
  if timeinstk() == 64 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 30 then
    prints "FAIL pareq response checks did not complete: %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 10.1 0 1 0.25 0.70710678
i 1 0 10.1 0 1 0.25 2
i 1 0 10.1 0 1 4 0.70710678
i 1 0 10.1 0 1 4 2
i 1 0 10.1 0 4095 0.25 0.70710678
i 1 0 10.1 0 4095 0.25 2
i 1 0 10.1 0 4095 4 0.70710678
i 1 0 10.1 0 4095 4 2
i 1 0 10.1 1 1 0.25 0.70710678
i 1 0 10.1 1 1 0.25 2
i 1 0 10.1 1 1 4 0.70710678
i 1 0 10.1 1 1 4 2
i 1 0 10.1 1 4095 0.25 0.70710678
i 1 0 10.1 1 4095 0.25 2
i 1 0 10.1 1 4095 4 0.70710678
i 1 0 10.1 1 4095 4 2
i 1 0 10.1 2 1 0.25 0.70710678
i 1 0 10.1 2 1 0.25 2
i 1 0 10.1 2 1 4 0.70710678
i 1 0 10.1 2 1 4 2
i 1 0 10.1 2 4095 0.25 0.70710678
i 1 0 10.1 2 4095 0.25 2
i 1 0 10.1 2 4095 4 0.70710678
i 1 0 10.1 2 4095 4 2
i 2 0 .1 0
i 2 0 .1 1
i 2 0 .1 2
i 3 0 1 0 1
i 3 0 1 1 .25
i 3 0 1 2 .25
i 99 10.2 .001
e
</CsScore>
</CsoundSynthesizer>
