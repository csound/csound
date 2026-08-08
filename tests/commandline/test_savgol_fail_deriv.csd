<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Expected failure: the coefficient matrix only holds order + 1 derivative
; rows, so a deriv above the polynomial order has nothing to read.

instr 1
  prints("expecting: deriv must be >= 0 and <= order\n")
  y:a = savgol(a(1), 9, 2, 3)
  out(y)
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
