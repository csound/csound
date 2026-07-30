<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Expected failure: a Savitzky-Golay window has to be centred on a sample, so
; an even winsize must be rejected at init time.

instr 1
  prints("expecting: winsize must be odd and at least 3\n")
  y:a = savgol(a(1), 4, 2)
  out(y)
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
