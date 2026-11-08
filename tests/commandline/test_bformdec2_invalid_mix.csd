<CsTest>
description = "bformdec2 rejects an invalid mix selector before building its matrices"

[expect]
exit = "nonzero"
stderr = ["bformdec2: mix type must be 0, 1 or 2"]
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 2
0dbfs = 1

instr 1
  aInput[] init 4
  aOutput[] init 4
  ; Quad layout accepts mix selectors 0, 1 and 2. The next value is invalid.
  aOutput bformdec2 2, aInput, 0, -1, 400, 3
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
