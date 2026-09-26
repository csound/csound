<CsTest>
description = "Audio-array arithmetic with scalars and control arrays respects local element strides"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 32
gkChecks init 0

; Check both elements and every sample, including later local blocks.
opcode CheckPair, 0, a[]kkS
  aValues[], kFirst, kSecond, SOperation xin
  kIndex = 0
  while kIndex < ksmps do
    kActualFirst vaget kIndex, aValues[0]
    kActualSecond vaget kIndex, aValues[1]
    if !(abs(kActualFirst-kFirst) < .00001 && \
         abs(kActualSecond-kSecond) < .00001) then
      printks "%s, sample %g: expected [%g, %g], got [%g, %g]\n", \
        0, SOperation, kIndex, kFirst, kSecond, kActualFirst, kActualSecond
      exitnowk -1
    endif
    kIndex += 1
  od
endop

opcode CheckArithmetic, 0, 0
  setksmps 2
  aLocal[] init 2
  aLocal[0] = 2
  aLocal[1] = 4
  kScalar = 3
  aScalar = 3
  kArray[] fillarray 3, 5

  ; Operations with kScalar, in both operand orders.
  aControlSum[] = aLocal + kScalar
  CheckPair aControlSum, 5, 7, "aLocal + kScalar"
  aControlReverseSum[] = kScalar + aLocal
  CheckPair aControlReverseSum, 5, 7, "kScalar + aLocal"
  aControlDifference[] = aLocal - kScalar
  CheckPair aControlDifference, -1, 1, "aLocal - kScalar"
  aControlReverseDifference[] = kScalar - aLocal
  CheckPair aControlReverseDifference, 1, -1, "kScalar - aLocal"
  aControlProduct[] = aLocal * kScalar
  CheckPair aControlProduct, 6, 12, "aLocal * kScalar"
  aControlReverseProduct[] = kScalar * aLocal
  CheckPair aControlReverseProduct, 6, 12, "kScalar * aLocal"
  aControlQuotient[] = aLocal / kScalar
  CheckPair aControlQuotient, 2/3, 4/3, "aLocal / kScalar"
  aControlReverseQuotient[] = kScalar / aLocal
  CheckPair aControlReverseQuotient, 1.5, .75, "kScalar / aLocal"
  aControlRemainder[] = aLocal % kScalar
  CheckPair aControlRemainder, 2, 1, "aLocal % kScalar"
  aControlPower[] = aLocal ^ kScalar
  CheckPair aControlPower, 8, 64, "aLocal ^ kScalar"

  ; Operations with aScalar, in both operand orders.
  aAudioSum[] = aLocal + aScalar
  CheckPair aAudioSum, 5, 7, "aLocal + aScalar"
  aAudioReverseSum[] = aScalar + aLocal
  CheckPair aAudioReverseSum, 5, 7, "aScalar + aLocal"
  aAudioDifference[] = aLocal - aScalar
  CheckPair aAudioDifference, -1, 1, "aLocal - aScalar"
  aAudioReverseDifference[] = aScalar - aLocal
  CheckPair aAudioReverseDifference, 1, -1, "aScalar - aLocal"
  aAudioProduct[] = aLocal * aScalar
  CheckPair aAudioProduct, 6, 12, "aLocal * aScalar"
  aAudioReverseProduct[] = aScalar * aLocal
  CheckPair aAudioReverseProduct, 6, 12, "aScalar * aLocal"
  aAudioQuotient[] = aLocal / aScalar
  CheckPair aAudioQuotient, 2/3, 4/3, "aLocal / aScalar"
  aAudioReverseQuotient[] = aScalar / aLocal
  CheckPair aAudioReverseQuotient, 1.5, .75, "aScalar / aLocal"

  ; Operations with kArray, in both operand orders.
  aControlArraySum[] = aLocal + kArray
  CheckPair aControlArraySum, 5, 9, "aLocal + kArray"
  aControlArrayReverseSum[] = kArray + aLocal
  CheckPair aControlArrayReverseSum, 5, 9, "kArray + aLocal"
  aControlArrayDifference[] = aLocal - kArray
  CheckPair aControlArrayDifference, -1, -1, "aLocal - kArray"
  aControlArrayReverseDifference[] = kArray - aLocal
  CheckPair aControlArrayReverseDifference, 1, 1, "kArray - aLocal"
  aControlArrayProduct[] = aLocal * kArray
  CheckPair aControlArrayProduct, 6, 20, "aLocal * kArray"
  aControlArrayReverseProduct[] = kArray * aLocal
  CheckPair aControlArrayReverseProduct, 6, 20, "kArray * aLocal"
  aControlArrayQuotient[] = aLocal / kArray
  CheckPair aControlArrayQuotient, 2/3, .8, "aLocal / kArray"
  aControlArrayReverseQuotient[] = kArray / aLocal
  CheckPair aControlArrayReverseQuotient, 1.5, 1.25, "kArray / aLocal"
  aControlArrayRemainder[] = aLocal % kArray
  CheckPair aControlArrayRemainder, 2, 4, "aLocal % kArray"
  aControlArrayPower[] = aLocal ^ kArray
  CheckPair aControlArrayPower, 8, 1024, "aLocal ^ kArray"
  aReverseRemainder[] = kArray % aLocal
  CheckPair aReverseRemainder, 1, 1, "kArray % aLocal"

endop

instr RunChecks
  CheckArithmetic
  gkChecks += 1
  turnoff
endin

instr CheckResults
  if i(gkChecks) != 1 then
    prints "Audio-array arithmetic checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "RunChecks" 0 .25
i "CheckResults" .5 .25
e
</CsScore>
</CsoundSynthesizer>
