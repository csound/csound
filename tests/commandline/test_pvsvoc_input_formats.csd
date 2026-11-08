<CsTest>
description = "pvsvoc rejects mismatched and sliding input spectra during initialization"

[expect]
exit = "nonzero"
stderr = ["pvsvoc: input spectra must have matching formats", "pvsvoc: sliding analysis is not supported"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
 fFirst pvsinit 128, 32, 128, 1, 0
 fSecond pvsinit p4, p5, p6, p7, p8
 fOutput pvsvoc fFirst, fSecond, 1, 1
endin

instr 2
 fSliding pvsinit 128, 1, 128, 1, 0
 fFixed pvsinit 128, 32, 128, 1, 0
 if p4 == 0 then
  fOutput pvsvoc fSliding, fFixed, 1, 1
 else
  fOutput pvsvoc fFixed, fSliding, 1, 1
 endif
endin
</CsInstruments>
<CsScore>
; Change one property of the second input at a time:
; FFT size, hop size, window size, window type, then spectral format.
i 1 0 .01 256 32 128 1 0
i 1 0 .01 128 64 128 1 0
i 1 0 .01 128 32 256 1 0
i 1 0 .01 128 32 128 0 0
i 1 0 .01 128 32 128 1 1
; Neither input may use sliding analysis.
i 2 0 .01 0
i 2 0 .01 1
e
</CsScore>
</CsoundSynthesizer>
