<CsoundSynthesizer>
<CsOptions>
-odac -m128
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 64
nchnls = 2
0dbfs = 1

//test for +=, -=, *=, /= on audio arrays

instr 1
 gaArr1[] init 2
 gaArr1[0] poscil .2, 400
 gaArr1[1] poscil .2, 500
 gaArr2[] init 2
 gaArr2[0] poscil .2, 600
 gaArr2[1] poscil .2, 700 
endin

instr 2
 aOutArr[] = gaArr1
 aOutArr += gaArr2
 outch p4, aOutArr[p4-1]
endin

instr 3
 aOutArr[] = gaArr1
 aOutArr *= gaArr2
 outch p4, aOutArr[p4-1]
endin

instr 4
 aOutArr[] = gaArr1
 aOutArr -= gaArr1 //results in silence
 outch p4, aOutArr[p4-1]
endin

instr 5
 aOutArr[] = gaArr1
 aOutArr /= (gaArr2+1)
 outch p4, aOutArr[p4-1]
endin

instr 6
 aValues[] init 1
 aValues[0] = 5
 kAmount = 2
 aValues -= kAmount
 kValue downsamp aValues[0]
 if kValue != 3 then
  printks "a[] -= k expected 3, got %f\n", 0, kValue
  exitnowk(-1)
 endif
endin

instr 7
  aDenominators[] init 2
  aDenominators[0] = 2
  aDenominators[1] = 4
  aNumerators[] init 2
  aNumerators[0] = 8
  aNumerators[1] = 12
  aNumerator = 8
  aDenominator = 2
  aFactor = 3

  aScalarDivArray[] = aNumerator / aDenominators
  aArrayDivScalar[] = aNumerators / aDenominator
  aScalarMulArray[] = aFactor * aDenominators

  kScalarDiv0 downsamp aScalarDivArray[0]
  kScalarDiv1 downsamp aScalarDivArray[1]
  kArrayDiv0 downsamp aArrayDivScalar[0]
  kArrayDiv1 downsamp aArrayDivScalar[1]
  kScalarMul0 downsamp aScalarMulArray[0]
  kScalarMul1 downsamp aScalarMulArray[1]

  if timeinstk() == 1 then
    if kScalarDiv0 != 4 || kScalarDiv1 != 2 then
      printks "a / a[] failed: got [%f, %f]\n", 0, \
               kScalarDiv0, kScalarDiv1
      exitnowk(-1)
    endif
    if kArrayDiv0 != 4 || kArrayDiv1 != 6 then
      printks "a[] / a failed: got [%f, %f]\n", 0, \
               kArrayDiv0, kArrayDiv1
      exitnowk(-1)
    endif
    if kScalarMul0 != 6 || kScalarMul1 != 12 then
      printks "a * a[] failed: got [%f, %f]\n", 0, \
               kScalarMul0, kScalarMul1
      exitnowk(-1)
    endif
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 16
i 6 0 0.01
i 2 0 2 1
i 2 2 2 2
i 3 4 2 1
i 3 6 2 2
i 4 8 2 1
i 4 10 2 2
i 5 12 2 1
i 5 14 2 2
i 7 0 .1
</CsScore>
</CsoundSynthesizer>
