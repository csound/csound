<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

nchnls = 1
0dbfs = 1

opcode assert_close_i, 0, iii
 iActual, iExpected, iTolerance xin
 iDifference = abs(iActual - iExpected)
 if qnan(iDifference) != 0 || iDifference > iTolerance then
  prints "assert_close_i failed: actual=%f expected=%f\n", iActual, iExpected
  exitnow(-1)
 endif
endop

instr 1
 kArr[] genarray_i 0,1023
 spec:Complex[] fft kArr
 kArr fft spec
endin

instr 2
 iComplexInput:Complex[] = [complex(1, 0), complex(0, 1), \
                            complex(-1, 0), complex(0, -1)]
 iRealInput[] = [1, 0, 0, 0]

 iComplexForward:Complex[] = fft(iComplexInput)
 iRealForward:Complex[] = fft(iRealInput)
 iComplexInverse:Complex[] = fft(iComplexInput, 1)
 iRealInverse[] = fft(iRealForward)

 if lenarray(iComplexForward) != 4 || lenarray(iRealForward) != 4 || \
    lenarray(iComplexInverse) != 4 || lenarray(iRealInverse) != 4 then
  prints "typed init fft returned an invalid array size\n"
  exitnow(-1)
 endif

 iTolerance = 0.000001
 iIndex = 0
 while iIndex < 4 do
  iExpectedForward = iIndex == 1 ? 4 : 0
  iExpectedInverse = iIndex == 3 ? 1 : 0
  iExpectedRealInverse = iIndex == 0 ? 1 : 0

  iComplexForwardReal = real(iComplexForward[iIndex])
  iComplexForwardImag = imag(iComplexForward[iIndex])
  iRealForwardReal = real(iRealForward[iIndex])
  iRealForwardImag = imag(iRealForward[iIndex])
  iComplexInverseReal = real(iComplexInverse[iIndex])
  iComplexInverseImag = imag(iComplexInverse[iIndex])

  assert_close_i(iComplexForwardReal, iExpectedForward, iTolerance)
  assert_close_i(iComplexForwardImag, 0, iTolerance)
  assert_close_i(iRealForwardReal, 1, iTolerance)
  assert_close_i(iRealForwardImag, 0, iTolerance)
  assert_close_i(iComplexInverseReal, iExpectedInverse, iTolerance)
  assert_close_i(iComplexInverseImag, 0, iTolerance)
  assert_close_i(iRealInverse[iIndex], iExpectedRealInverse, iTolerance)
  iIndex += 1
 od
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 0
</CsScore>
</CsoundSynthesizer>
