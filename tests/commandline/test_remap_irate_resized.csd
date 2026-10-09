<CsTest>
description = "remap checks i-rate table lengths during performance"

[expect]
exit = 1
stderr = ["[remap] x and y must have same length"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; i-rate tables pass their checks at init, but a global i-rate array can
; still be resized by another instrument while the note runs. Every call
; checks the lengths again, so the shrunk x table stops the lookup instead
; of reading past its end.

#define MODE_LINEAR  # 0 #
#define BOUNDS_CLAMP # 1 #

gxd@global:i[] = fillarray(0, 1, 2, 3)
gyd@global:i[] = fillarray(0, 10, 20, 30)

instr 1
  y:k = remap(k(2.5), gxd, gyd, $MODE_LINEAR, $BOUNDS_CLAMP)
endin

instr 2
  gxd = fillarray(0, 1)
endin

</CsInstruments>
<CsScore>
i1 0   0.5
i2 0.1 0.1
e
</CsScore>
</CsoundSynthesizer>
