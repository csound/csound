<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kBlock init 0
 kBlock += 1
 ; Cross zero with live filter history, including negative and tiny values.
 if kBlock < 3 then
  kSaturation = 0
 elseif kBlock < 5 then
  kSaturation = 2
 elseif kBlock < 7 then
  kSaturation = (p6 == 0 ? 1e-20 : p6)
 elseif kBlock < 9 then
  kSaturation = -2
 else
  kSaturation = 0
 endif
 aInput oscili .5, 1000
 aCutoff = 1000
 aFeedback = 2

 ; Applying the nonlinearity before the linear filter must give the same
 ; result, including state carried across changes in saturation.
 if p4 == 1 && kSaturation != 0 then
  aShaped = tanh(kSaturation*aInput)/tanh(kSaturation)
 elseif p4 == 2 then
  aShaped = tanh(kSaturation*aInput)
 else
  aShaped = aInput
 endif
 aExpected diode_ladder aShaped, 1000, 2, 0
 if p5 == 0 then
  aActual diode_ladder aInput, 1000, 2, p4, kSaturation
 elseif p5 == 1 then
  aActual diode_ladder aInput, aCutoff, 2, p4, kSaturation
 elseif p5 == 2 then
  aActual diode_ladder aInput, 1000, aFeedback, p4, kSaturation
 elseif p5 == 3 then
  aActual diode_ladder aInput, aCutoff, aFeedback, p4, kSaturation
 else
  aInput diodeladder aInput, aCutoff, aFeedback, p4, kSaturation
  aActual = aInput
 endif
 kN = 0
 while kN < ksmps do
  kActual vaget kN, aActual
  kExpected vaget kN, aExpected
  if !(abs(kActual-kExpected) < .000003) then
   printks "diode_ladder mode=%g rates=%g block=%g sample=%g actual=%g expected=%g\n", 0, p4, p5, kBlock, kN, kActual, kExpected
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 12 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 13 then
  prints "diode_ladder checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; All input parameter rates, the alias, and input reuse.
i 1 0 .025 1 0
i 1 0 .025 1 1
i 1 0 .025 1 2
i 1 0 .025 1 3
i 1 0 .025 1 4
i 1 0 .025 0 0
i 1 0 .025 2 0
i 1 0 .025 2 4
; A tiny double-precision value whose reciprocal would overflow.
i 1 0 .025 1 0 1e-310
; Partial first and last blocks.
i 1 .030625 .02525 1 0
i 1 .030625 .02525 1 3
i 1 .030625 .02525 1 4
i 1 .030625 .02525 2 3
i 99 .06 .001
e
</CsScore>
</CsoundSynthesizer>
