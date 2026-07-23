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
 spec:Complex[] rfft kArr
 kArr rifft spec
endin

instr 2
 iInput[] = [1, 0, 0, 0]
 iSpectrum:Complex[] = rfft(iInput)
 iOutput[] = rifft(iSpectrum)

 if lenarray(iSpectrum) != 3 || lenarray(iOutput) != 4 then
  prints "typed init rfft returned an invalid array size\n"
  exitnow(-1)
 endif

 iTolerance = 0.000001
 iIndex = 0
 while iIndex < lenarray(iSpectrum) do
  iSpectrumReal = real(iSpectrum[iIndex])
  iSpectrumImag = imag(iSpectrum[iIndex])
  assert_close_i(iSpectrumReal, 1, iTolerance)
  assert_close_i(iSpectrumImag, 0, iTolerance)
  iIndex += 1
 od

 iIndex = 0
 while iIndex < lenarray(iOutput) do
  iExpected = iIndex == 0 ? 1 : 0
  assert_close_i(iOutput[iIndex], iExpected, iTolerance)
  iIndex += 1
 od
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 0
</CsScore>
</CsoundSynthesizer>
