<CsTest>
description = "remap rejects i-rate x data that is not strictly increasing"

[expect]
exit = 1
stderr = ["[remap] x data must be strictly increasing"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Both tables are i-rate, but Csound resolves this call to the overload that
; declares k[] tables: the check must look at the arrays, not the overload.
; A repeated x breakpoint would otherwise divide by zero and return nan.

#define MODE_LINEAR  # 0 #
#define BOUNDS_CLAMP # 1 #

instr 1
  xd:i[] = fillarray(0, 0)
  yd:i[] = fillarray(0, 10)
  y:k = remap(0, xd, yd, $MODE_LINEAR, $BOUNDS_CLAMP)
  printf("remap: repeated x breakpoint accepted, got %f\n", 1, y)
endin

</CsInstruments>
<CsScore>
i1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
