<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 32
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
  kTolerance = 0.00001
  kRectSpectrum:Complex[] = [complex(1, sqrt(3)), complex(0, 1), \
                             complex(-2, 2 * sqrt(3))]
  kPolarSpectrum:Complex[] = [complex(2, $M_PI / 3, 1), \
                              complex(1, $M_PI / 2, 1), \
                              complex(4, 2 * $M_PI / 3, 1)]
  kRectOutput[] rifft kRectSpectrum
  kPolarOutput[] rifft kPolarSpectrum
  kExpectedOutput[] = [-0.25, 0.25, -0.25, 1.25]

  kIndex = 0
  while kIndex < lenarray(kExpectedOutput) do
    assert_close(kRectOutput[kIndex], kExpectedOutput[kIndex], kTolerance)
    assert_close(kPolarOutput[kIndex], kExpectedOutput[kIndex], kTolerance)
    kIndex += 1
  od

  kInput[] = [-2, 1, 0, 0]
  kExpectedSpectrum:Complex[] = [complex(-1, 0), complex(-2, -1), \
                                 complex(-3, 0)]
  kReusedSpectrum:Complex[] = [complex(1, 0, 1), complex(1, 0, 1), \
                               complex(1, 0, 1)]
  kReusedSpectrum rfft kInput

  kIndex = 0
  while kIndex < lenarray(kExpectedSpectrum) do
    assert_close(real(kReusedSpectrum[kIndex]), \
                 real(kExpectedSpectrum[kIndex]), kTolerance)
    assert_close(imag(kReusedSpectrum[kIndex]), \
                 imag(kExpectedSpectrum[kIndex]), kTolerance)
    assert_close(abs(kReusedSpectrum[kIndex]), \
                 abs(kExpectedSpectrum[kIndex]), kTolerance)
    assert_close(arg(kReusedSpectrum[kIndex]), \
                 arg(kExpectedSpectrum[kIndex]), kTolerance)
    kIndex += 1
  od
  turnoff
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
