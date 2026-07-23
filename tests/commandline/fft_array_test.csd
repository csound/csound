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
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
