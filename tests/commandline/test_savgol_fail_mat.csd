<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Expected failure: savgolmat validates its arguments the same way the filter
; does, so an even winsize is rejected there too.

instr 1
  prints("expecting: winsize must be odd and at least 3\n")
  mat:i[][] = savgolmat(6, 2)
  prints("unreachable: %d rows\n", lenarray(mat, 1))
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
