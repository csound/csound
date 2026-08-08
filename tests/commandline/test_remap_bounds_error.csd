<CsoundSynthesizer>
<CsOptions>

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; remap with bounds = error must raise a performance error as soon as x falls
; outside the breakpoint table. The error aborts the event and csound returns
; a non-zero code, so this case cannot live alongside the other remap tests:
; it is registered as an expected failure instead.
;
; Everything else in this file is deliberately kept out, so a non-zero return
; can only come from the out-of-range lookup below.

#define MODE_LINEAR  # 0 #
#define BOUNDS_ERROR # 0 #

instr 1
  xd:i[] = fillarray(0, 1, 3, 4)
  yd:i[] = fillarray(0, 2, 2, 6)

  ; 5 is past the last breakpoint
  y:k = remap(k(5), xd, yd, $MODE_LINEAR, $BOUNDS_ERROR)
  printf("remap: bounds=error did not abort, got %f\n", 1, y)
endin

</CsInstruments>
<CsScore>
i1 0 0.5
e
</CsScore>
</CsoundSynthesizer>
