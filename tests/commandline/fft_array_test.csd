<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

opcode assert_close, 0, kkk
  kActual, kExpected, kTolerance xin
  kDifference = abs(kActual - kExpected)
  if qnan(kDifference) != 0 || kDifference > kTolerance then
    printks "assert_close failed: actual=%f expected=%f difference=%f\n", \
            1, kActual, kExpected, kDifference
    exitnowk(-1)
  endif
endop

instr 1
  kInput:Complex[] = [complex(0, 0), complex(1, 2), \
                      complex(0, 0), complex(0, 0), \
                      complex(0, 0), complex(0, 0)]
  kSpectrum:Complex[] = fft(kInput)
  kRoundTrip:Complex[] = fft(kSpectrum, 1)

  kIndex = 0
  while kIndex < lenarray(kInput) do
    kAngle = 2 * $M_PI * kIndex / lenarray(kInput)
    kExpectedReal = cos(kAngle) + 2 * sin(kAngle)
    kExpectedImag = 2 * cos(kAngle) - sin(kAngle)
    assert_close(real(kSpectrum[kIndex]), kExpectedReal, 0.00001)
    assert_close(imag(kSpectrum[kIndex]), kExpectedImag, 0.00001)
    assert_close(real(kRoundTrip[kIndex]), kIndex == 1 ? 1 : 0, 0.00001)
    assert_close(imag(kRoundTrip[kIndex]), kIndex == 1 ? 2 : 0, 0.00001)
    kIndex += 1
  od

  kRectInput:Complex[] = [complex(1, 2), complex(-3, 0.5), \
                          complex(0, -1), complex(2, -4)]
  kPolarInput:Complex[] = [polar(kRectInput[0]), polar(kRectInput[1]), \
                           polar(kRectInput[2]), polar(kRectInput[3])]
  kRectSpectrum:Complex[] = fft(kRectInput)
  kPolarSpectrum:Complex[] = [polar(kRectSpectrum[0]), \
                              polar(kRectSpectrum[1]), \
                              polar(kRectSpectrum[2]), \
                              polar(kRectSpectrum[3])]
  kPolarInputSpectrum:Complex[] = fft(kPolarInput)

  kRectOutput:Complex[] = fftinv(kRectSpectrum)
  kFlagOutput:Complex[] = fft(kRectSpectrum, 1)
  kPolarOutput:Complex[] = fftinv(kPolarSpectrum)
  kPolarInputOutput:Complex[] = fftinv(kPolarInputSpectrum)

  kIndex = 0
  while kIndex < lenarray(kRectInput) do
    kExpectedReal = real(kRectInput[kIndex])
    kExpectedImag = imag(kRectInput[kIndex])
    assert_close(real(kRectOutput[kIndex]), kExpectedReal, 0.00001)
    assert_close(imag(kRectOutput[kIndex]), kExpectedImag, 0.00001)
    assert_close(real(kFlagOutput[kIndex]), kExpectedReal, 0.00001)
    assert_close(imag(kFlagOutput[kIndex]), kExpectedImag, 0.00001)
    assert_close(real(kPolarOutput[kIndex]), kExpectedReal, 0.00001)
    assert_close(imag(kPolarOutput[kIndex]), kExpectedImag, 0.00001)
    assert_close(real(kPolarInputOutput[kIndex]), kExpectedReal, 0.00001)
    assert_close(imag(kPolarInputOutput[kIndex]), kExpectedImag, 0.00001)
    kIndex += 1
  od

  kUnitSpectrum:Complex[] = [complex(1, 0), complex(1, 0), \
                             complex(1, 0), complex(1, 0)]
  kComplexOutput:Complex[] = fftinv(kUnitSpectrum)
  kRealOutput[] = fftinv(kUnitSpectrum)
  kFftRealOutput[] = fft(kUnitSpectrum)
  assert_close(real(kComplexOutput[0]), 1, 0.00001)
  assert_close(imag(kComplexOutput[0]), 0, 0.00001)
  assert_close(kRealOutput[0], 1, 0.00001)
  assert_close(kFftRealOutput[0], 1, 0.00001)
  kIndex = 1
  while kIndex < lenarray(kUnitSpectrum) do
    assert_close(real(kComplexOutput[kIndex]), 0, 0.00001)
    assert_close(imag(kComplexOutput[kIndex]), 0, 0.00001)
    assert_close(kRealOutput[kIndex], 0, 0.00001)
    assert_close(kFftRealOutput[kIndex], 0, 0.00001)
    kIndex += 1
  od

  kRealInput[] = [1, 2, 3, 4]
  kRealSpectrum:Complex[] = fft(kRealInput)
  kRealRoundTrip[] = fftinv(kRealSpectrum)
  kIndex = 0
  while kIndex < lenarray(kRealInput) do
    assert_close(kRealRoundTrip[kIndex], kRealInput[kIndex], 0.00001)
    kIndex += 1
  od
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
