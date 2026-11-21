<CsTest>
description = "PFFFT keeps the build's sample precision in FFT and DCT operations"
[expect]
exit = 0
output = ["Total assertions: 130", "Failed: 0"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --run-unit-tests --fftlib=1
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  ; In a double build, the alternating step is too small for float storage.
  ; A float build uses a larger step that its sample type can represent.
  iStep = (floatsize() == 8 ? 2^-30 : 2^-12)
  iTolerance = (floatsize() == 8 ? 1e-12 : 1e-5)
  iInput[] init 64
  iSample = 0
  while iSample < 64 do
    iInput[iSample] = 1 + (iSample % 2) * iStep
    iSample += 1
  od

  iSpectrum[] rfft iInput
  ; The packed real FFT stores the Nyquist bin at index 1.
  assert(abs(iSpectrum[1] - (-32 * iStep)) < iTolerance)
  iRestored[] rifft iSpectrum

  iCosines[] dct iInput
  ; DCT-II stores twice the input sum in its first coefficient.
  assert(abs(iCosines[0] - (128 + 64 * iStep)) < iTolerance)
  iRestoredDct[] dctinv iCosines

  iSample = 0
  while iSample < 64 do
    assert(abs(iRestored[iSample] - iInput[iSample]) < iTolerance)
    assert(abs(iRestoredDct[iSample] - iInput[iSample]) < iTolerance)
    iSample += 1
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
