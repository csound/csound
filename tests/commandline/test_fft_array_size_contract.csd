<CsTest>
description = "array FFT opcodes reject changed and invalid transform sizes"

[expect]
exit = "nonzero"
stderr = [
  "rfft: input size changed; reinitialise the opcode",
  "cmplxprod: array size must be even and at least 2",
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 16

instr RfftResize
  kInput[] fillarray 1, 2, 3, 4, 5, 6, 7, 8
  trimi kInput, 2
  if timeinstk() == 2 then
    trim kInput, 8
  endif
  kSpectrum:Complex[] = rfft(kInput)
endin

instr OddCmplxprod
  kLeft[] fillarray 1, 2, 3
  kRight[] fillarray 4, 5, 6
  kOutput[] cmplxprod kLeft, kRight
endin
</CsInstruments>
<CsScore>
i "RfftResize" 0 0.01
i "OddCmplxprod" 0.02 0.01
</CsScore>
</CsoundSynthesizer>
