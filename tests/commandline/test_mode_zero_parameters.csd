<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode ModeReference, a, aaa
 setksmps 1
 aIn, aFrequency, aQ xin
 kIn downsamp aIn
 kFrequency downsamp aFrequency
 kQ downsamp aQ
 kX init 0
 kY1 init 0
 kY2 init 0
 kOut = 0
 if kFrequency == 0 || kQ == 0 then
  kY1 = 0
  kY2 = 0
 else
  ; Equivalent coefficients expressed in normalized frequency, without
  ; the production code's reciprocal-frequency intermediates.
  kW = 2*$M_PI*kFrequency/sr
  kR = kQ/(kQ+kW/2)
  kY = kR*kW*kW*kX-kR*(kW*kW-2)*kY1-(2*kR-1)*kY2
  kY2 = kY1
  kY1 = kY
  kOut = kY/(2*kW)
 endif
 kX = kIn
 aOut = kOut
 xout aOut
endop

instr 1
 kBlock init 0
 kBlock += 1
 kZeroBlock = (p6 == 1 ? 1 : 2)
 kFrequency = (kBlock == kZeroBlock && p4 != 1 ? 0 : 440)
 kQ = (kBlock == kZeroBlock && p4 != 0 ? 0 : 10)
 aFrequency = kFrequency
 aQ = kQ
 aInput = .1
 aReuse = aInput
 if p5 == 0 then
  aOut mode aInput, kFrequency, kQ
  aReuse mode aReuse, kFrequency, kQ
 elseif p5 == 1 then
  aOut mode aInput, aFrequency, kQ
  aReuse mode aReuse, aFrequency, kQ
 elseif p5 == 2 then
  aOut mode aInput, kFrequency, aQ
  aReuse mode aReuse, kFrequency, aQ
 else
  aOut mode aInput, aFrequency, aQ
  aReuse mode aReuse, aFrequency, aQ
 endif
 aExpected ModeReference aInput, aFrequency, aQ
 kN = 0
 while kN < ksmps do
  kOut vaget kN, aOut
  kExpected vaget kN, aExpected
  kReuse vaget kN, aReuse
  if !(abs(kOut-kExpected) < .00002 && abs(kReuse-kExpected) < .00002) then
   printks "mode zero=%g rates=%g block=%g sample=%g output=%g expected=%g reuse=%g\n", 0, p4, p5, kBlock, kN, kOut, kExpected, kReuse
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 2
 ; Zero for a single sample within each block, with recovery on the next
 ; sample. Alternate between zero frequency and zero Q across notes.
 aFrequency = 440
 aQ = 10
 if p4 == 0 then
  vaset 0, 8, aFrequency
 else
  vaset 0, 8, aQ
 endif
 aInput = .1
 aOut mode aInput, aFrequency, aQ
 aExpected ModeReference aInput, aFrequency, aQ
 kN = 0
 while kN < ksmps do
  kOut vaget kN, aOut
  kExpected vaget kN, aExpected
  if !(abs(kOut-kExpected) < .00002) then
   printks "mode single zero=%g sample=%g output=%g expected=%g\n", 0, p4, kN, kOut, kExpected
   exitnowk(-1)
  endif
  kN += 1
 od
 kBlock init 0
 kBlock += 1
 if kBlock == 4 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 17 then
  prints "mode cases did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Frequency, Q, and both zero; each control/audio parameter combination.
; Alternate full blocks with onset at sample 3 and an early final block.
i 1 0 .0078125 0 0
i 1 .0159912109375 .0068359375 0 1
i 1 .03125 .0078125 0 2
i 1 .0472412109375 .0068359375 0 3
i 1 .0625 .0078125 1 0
i 1 .0784912109375 .0068359375 1 1
i 1 .09375 .0078125 1 2
i 1 .1097412109375 .0068359375 1 3
i 1 .125 .0078125 2 0
i 1 .1409912109375 .0068359375 2 1
i 1 .15625 .0078125 2 2
i 1 .1722412109375 .0068359375 2 3
i 2 .1875 .0078125 0
i 2 .203125 .0078125 1
; Start with zero frequency, zero Q, or both, then enable the resonator.
i 1 .21875 .0078125 0 0 1
i 1 .234375 .0078125 1 3 1
i 1 .25 .0078125 2 1 1
i 99 .28 .001
e
</CsScore>
</CsoundSynthesizer>
