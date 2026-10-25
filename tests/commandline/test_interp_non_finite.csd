<CsTest>
description = "a() and interp go on from their input after a NaN or an infinity, and a ramp ends on its input"

[expect]
exit = 0
output = ["every ramp came back to its input"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

; interp - what a() of a k value runs - started each block from the value it
; had reached, never from its input: one NaN in, and every sample after it
; was NaN to the end of the note, the input finite all the while; an infinity
; the same, through inf - inf.
gkFailed init 0

opcode Recovers(name:S, sig:a, want:k, at:k):void
  if timeinstk() == at then
    first:k = vaget(0, sig)
    last:k = vaget(ksmps - 1, sig)
    if first != want || last != want then
      printf("%s: %.17g and %.17g at k-period %d, not %.17g\n", 1, name, first, last, at, want)
      gkFailed += 1
    endif
  endif
endop

instr Bad
  zero:k = 0
  nan:k = (timeinstk() == 2) ? zero / zero : 0.5
  inf:k = (timeinstk() == 2) ? 1 / zero : 0.5
  Recovers("a() after a NaN", a(nan), 0.5, 4)
  Recovers("a() after an infinity", a(inf), 0.5, 4)
  Recovers("interp after a NaN", interp(nan), 0.5, 4)
  Recovers("interp after an infinity", interp(inf), 0.5, 4)
endin

; A ramp to a third, then held: each block began on the sum of the last
; one's increments, which missed the third by their rounding, so the held
; value was never quite the input.
instr Held
  third:k = (timeinstk() < 3) ? 0 : 1 / 3
  Recovers("a() held after a ramp", a(third), 1 / 3, 6)
endin

; The verdict at an init, which a k-rate if would not guard.
instr Check
  if i(gkFailed) != 0 then
    exitnow(1)
  endif
  prints("every ramp came back to its input\n")
endin
</CsInstruments>
<CsScore>
i "Bad" 0 0.02
i "Held" 0 0.02
i "Check" 0.05 0.01
</CsScore>
</CsoundSynthesizer>
