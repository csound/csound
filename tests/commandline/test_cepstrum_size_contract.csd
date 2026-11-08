<CsTest>
description = "cepstrum opcodes reject changed and invalid transform sizes"

[expect]
exit = "nonzero"
stderr = [
  "cepsinv: input size changed; reinitialise the opcode",
  "ceps: FFT size must be a power of two and at least 64",
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
ksmps = 16

instr CepsinvResize
  kInput[] init 129
  trimi kInput, 65
  if timeinstk() == 2 then
    trim kInput, 129
  endif
  kOutput[] cepsinv kInput
endin

instr InvalidCepsSize
  kInput[] init 66
  kOutput[] ceps kInput, 0
endin
</CsInstruments>
<CsScore>
i "CepsinvResize" 0 0.01
i "InvalidCepsSize" 0.02 0.01
</CsScore>
</CsoundSynthesizer>
