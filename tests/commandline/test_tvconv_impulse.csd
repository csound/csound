<CsTest>
description = "tvconv frozen unit impulse preserves the input with the partition delay"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 32768
gkChecks init 0

instr CheckImpulse
 kSample init 0
 aInput oscili .25 * 0dbfs, 437
 ; Record a unit impulse followed by seven zeros, then freeze the filter.
 aImpulse = (kSample == 0 ? 0dbfs : 0)
 kWrite = (kSample < 8 ? 1 : 0)
 aOutput tvconv aInput, aImpulse, 1, kWrite, p4, 8
 ; Check the common in-place use as well as a separate output variable.
 aInPlace = aInput
 aInPlace tvconv aInPlace, aImpulse, 1, kWrite, p4, 8
 if p4 == 1 then
  aExpected = aInput
 else
  aExpected delay aInput, p4/sr
 endif
 kExpected downsamp aExpected
 kActual downsamp aOutput
 kInPlace downsamp aInPlace
 if !(abs(kActual-kExpected) + abs(kInPlace-kExpected) < .00001 * 0dbfs) then
  printks "tvconv impulse: partition=%g sample=%g expected=%g actual=%g in-place=%g\n", 0, p4, kSample, kExpected, kActual, kInPlace
  exitnowk(-1)
 endif
 kSample += 1
 if kSample == 256 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 3 then
  prints "tvconv impulse checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckImpulse" 0 .04 1
i "CheckImpulse" 0 .04 2
i "CheckImpulse" 0 .04 8
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
