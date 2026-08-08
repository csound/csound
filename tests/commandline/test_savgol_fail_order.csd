<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; Expected failure: the polynomial order must stay below the window size.
; order == winsize - 1 is the largest value accepted, so winsize itself is out.

instr 1
  prints("expecting: order must be >= 0 and less than winsize\n")
  y:a = savgol(a(1), 5, 5)
  out(y)
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
