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

  kTolerance = 0.00001
  kRectInput:Complex[] = [complex(1, 0), complex(0, 1), \
                          complex(-1, 0), complex(0, -1)]
  kPolarInput:Complex[] = [complex(1, 0, 1), \
                           complex(1, $M_PI / 2, 1), \
                           complex(1, $M_PI, 1), \
                           complex(1, -$M_PI / 2, 1)]

  kRectForward:Complex[] fft kRectInput
  kPolarForward:Complex[] fft kPolarInput
  kRectInverse:Complex[] fft kRectInput, 1
  kPolarInverse:Complex[] fft kPolarInput, 1
  kRectInverseReal[] fft kRectInput
  kPolarInverseReal[] fft kPolarInput

  kIndex = 0
  while kIndex < lenarray(kRectInput) do
    assert_close(real(kPolarForward[kIndex]), real(kRectForward[kIndex]), \
                 kTolerance)
    assert_close(imag(kPolarForward[kIndex]), imag(kRectForward[kIndex]), \
                 kTolerance)
    assert_close(real(kPolarInverse[kIndex]), real(kRectInverse[kIndex]), \
                 kTolerance)
    assert_close(imag(kPolarInverse[kIndex]), imag(kRectInverse[kIndex]), \
                 kTolerance)
    assert_close(real(kRectForward[kIndex]), kIndex == 1 ? 4 : 0, \
                 kTolerance)
    assert_close(imag(kRectForward[kIndex]), 0, kTolerance)
    assert_close(real(kRectInverse[kIndex]), kIndex == 3 ? 1 : 0, \
                 kTolerance)
    assert_close(imag(kRectInverse[kIndex]), 0, kTolerance)
    assert_close(kPolarInverseReal[kIndex], kRectInverseReal[kIndex], \
                 kTolerance)
    assert_close(kRectInverseReal[kIndex], kIndex == 3 ? 1 : 0, \
                 kTolerance)
    kIndex += 1
  od

  kNegative:Complex[] = [complex(-1, 0), complex(0, 0), \
                          complex(0, 0), complex(0, 0)]
  kReusedForward:Complex[] = [complex(1, 0, 1), complex(1, 0, 1), \
                              complex(1, 0, 1), complex(1, 0, 1)]
  kReusedInverse:Complex[] = [complex(1, 0, 1), complex(1, 0, 1), \
                              complex(1, 0, 1), complex(1, 0, 1)]
  kReusedForward fft kNegative
  kReusedInverse fft kNegative, 1

  kNegativeReal[] = [-1, 0, 0, 0]
  kReusedRealInput:Complex[] = [complex(1, 0, 1), complex(1, 0, 1), \
                                complex(1, 0, 1), complex(1, 0, 1)]
  kReusedRealInput fft kNegativeReal

  kIndex = 0
  while kIndex < lenarray(kNegative) do
    assert_close(real(kReusedForward[kIndex]), -1, kTolerance)
    assert_close(imag(kReusedForward[kIndex]), 0, kTolerance)
    assert_close(abs(arg(kReusedForward[kIndex])), $M_PI, kTolerance)
    assert_close(real(kReusedInverse[kIndex]), -0.25, kTolerance)
    assert_close(imag(kReusedInverse[kIndex]), 0, kTolerance)
    assert_close(abs(arg(kReusedInverse[kIndex])), $M_PI, kTolerance)
    assert_close(real(kReusedRealInput[kIndex]), -1, kTolerance)
    assert_close(imag(kReusedRealInput[kIndex]), 0, kTolerance)
    assert_close(abs(arg(kReusedRealInput[kIndex])), $M_PI, kTolerance)
    kIndex += 1
  od
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
