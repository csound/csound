<CsTest>
description = "barmodel rejects parameters that cannot form a finite grid"
[expect]
exit = "nonzero"
stderr = [
  "barmodel: stiffness and loss produce an invalid grid size",
  "barmodel: decay time must be positive",
  "barmodel: loss must be non-negative",
  "3 errors in performance"
]
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  aout barmodel 1, 1, p4, p5, .23, p6, .5, 1, .1
  out aout
endin
</CsInstruments>
<CsScore>
; Zero stiffness and loss make the spatial step zero.
i 1 0 .01 0 0 5
; A nonpositive decay time cannot form damping coefficients.
i 1 .02 .01 3 .001 0
; Negative loss can collapse the grid calculation.
i 1 .04 .01 3 -.001 5
e
</CsScore>
</CsoundSynthesizer>
