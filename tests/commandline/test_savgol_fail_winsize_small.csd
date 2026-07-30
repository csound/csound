<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Expected failure: a window shorter than 3 samples cannot support a fit.
; Checked through the k-rate entry point, which validates the same way.

instr 1
  prints("expecting: winsize must be odd and at least 3\n")
  y:k = savgol(k(1), 1, 0)
  printk2(y)
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
