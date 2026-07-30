<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Expected failure: delta is the sample spacing the derivative is scaled by,
; so zero would divide by zero rather than produce a finite result.

instr 1
  prints("expecting: delta must be greater than 0\n")
  y:a = savgol(a(1), 9, 2, 1, 0)
  out(y)
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
