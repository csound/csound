<CsTest>
description = "GEN51 rejects missing scale parameters, zero grades and fractional base keys"

[expect]
exit = "nonzero"
stderr = ["GEN51: insufficient arguments", "GEN51: invalid grade count or too few ratios", "GEN51: base key must be a 32-bit integer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1

instr 1
  iTable ftgen 1, 0, 8, -51, 12
endin

instr 2
  iTable ftgen 2, 0, 8, -51, 0, 2, 440, 69, 1
endin

instr 3
  iTable ftgen 3, 0, 8, -51, 1, 2, 440, 69.5, 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0 0.01
i 3 0 0.01
e
</CsScore>
</CsoundSynthesizer>
