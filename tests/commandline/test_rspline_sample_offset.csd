<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

gibounds ftgen 0, 0, 8, -2, 0, 0, 0, 0, 0, 1, 1, 1
gaBound init 0

instr 1
  aphase phasor 1000
  gaBound table aphase, gibounds, 1
endin

instr 2
  aresult rspline gaBound, gaBound, 10, 10
  kerror max_k abs(aresult - gaBound), 1, 1
  if kerror > .000000001 then
    printks "FAIL audio bounds shifted by %g\n", 0, kerror
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 .000625 .005
</CsScore>
</CsoundSynthesizer>
