<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 64
nchnls = 1
0dbfs = 1

gicapture ftgen 0, 0, 64, -2, 0

instr 1
  asignal = 1
  aresult gain asignal, 1, 100
  aindex line 0, p3, p3 * sr
  tablew aresult, aindex, gicapture
endin

instr 99
  ilast table 53, gicapture
  if !(ilast > 0) then
    prints "gain cleared an active sample at note end\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .00675
i 99 .02 .001
</CsScore>
</CsoundSynthesizer>
