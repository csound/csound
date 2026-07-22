<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
opcode assert_equal, 0, kk
  kActual, kExpected xin
  if kActual != kExpected then
    printks "array remainder error: actual=%f expected=%f\n", 1, \
            kActual, kExpected
    exitnowk(-1)
  endif
endop

instr 1 ;; Addition
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kS + kT
  printk2 kans[0]
  printk2 kans[1]
endin
instr 2 ;; Subtraction
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kT - kS
  printk2 kans[0]
  printk2 kans[1]
endin
instr 3 ;; Multiplication
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kT * kS
  printk2 kans[0]
  printk2 kans[1]
endin
instr 4 ;; Division
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kT[] init 2
  kT[0] = 3
  kT[1] = 4
  kans[] init 2
  kans = kT / kS
  printk2 kans[0]
  printk2 kans[1]
endin
instr 5 ;; Addition
  kS[] init 2
  kS[0] = 1
  kS[1] = 2
  kans[] init 2
  kans = kS + 2.5
  printk2 kans[0]
  printk2 kans[1]
endin

instr 6 ;; Remainder
  iLeft[] fillarray 5, 8
  iRight[] fillarray 2, 3
  iArrayArray[] = iLeft % iRight
  iArrayScalar[] = iLeft % 3
  iScalarArray[] = 8 % iRight
  assert_equal(iArrayArray[0], 1)
  assert_equal(iArrayArray[1], 2)
  assert_equal(iArrayScalar[0], 2)
  assert_equal(iArrayScalar[1], 2)
  assert_equal(iScalarArray[0], 0)
  assert_equal(iScalarArray[1], 2)

  kLeft[] fillarray 5, 8
  kRight[] fillarray 2, 3
  kDivisor = 3
  kDividend = 8
  kArrayArray[] = kLeft % kRight
  kArrayIScalar[] = kLeft % 3
  kIScalarArray[] = 8 % kRight
  kArrayKScalar[] = kLeft % kDivisor
  kKScalarArray[] = kDividend % kRight
  assert_equal(kArrayArray[0], 1)
  assert_equal(kArrayArray[1], 2)
  assert_equal(kArrayIScalar[0], 2)
  assert_equal(kArrayIScalar[1], 2)
  assert_equal(kIScalarArray[0], 0)
  assert_equal(kIScalarArray[1], 2)
  assert_equal(kArrayKScalar[0], 2)
  assert_equal(kArrayKScalar[1], 2)
  assert_equal(kKScalarArray[0], 0)
  assert_equal(kKScalarArray[1], 2)
  turnoff
endin

instr 7 ;; Audio-array remainder
  kLeft[] fillarray 5, 8
  kRight[] fillarray 2, 3
  kDivisor = 3
  aLeft[] init 2
  aRight[] init 2
  aLeft[0] = 5
  aLeft[1] = 8
  aRight[0] = 2
  aRight[1] = 3

  aKArrayAudioArray[] = kLeft % aRight
  aAudioArrayKArray[] = aLeft % kRight
  aAudioArrayKScalar[] = aLeft % kDivisor
  kKArrayAudioArray0 downsamp aKArrayAudioArray[0]
  kKArrayAudioArray1 downsamp aKArrayAudioArray[1]
  kAudioArrayKArray0 downsamp aAudioArrayKArray[0]
  kAudioArrayKArray1 downsamp aAudioArrayKArray[1]
  kAudioArrayKScalar0 downsamp aAudioArrayKScalar[0]
  kAudioArrayKScalar1 downsamp aAudioArrayKScalar[1]
  assert_equal(kKArrayAudioArray0, 1)
  assert_equal(kKArrayAudioArray1, 2)
  assert_equal(kAudioArrayKArray0, 1)
  assert_equal(kAudioArrayKArray1, 2)
  assert_equal(kAudioArrayKScalar0, 2)
  assert_equal(kAudioArrayKScalar1, 2)
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 0.1
i2 0.1 0.1
i3 0.2 0.1
i4 0.3 0.1
i5 0.4 0.1
i6 0.5 0.1
i7 0.6 0.1
</CsScore>
</CsoundSynthesizer>
