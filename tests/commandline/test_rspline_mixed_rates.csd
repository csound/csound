<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 64
nchnls = 1
0dbfs = 1

gireference ftgen 0, 0, 1024, -2, 0

instr 1
  seed 1
  kmin init 0
  kmax init 1
  areference rspline kmin, kmax, 100, 100
  aindex line 0, p3, p3 * sr
  tablew areference, aindex, gireference
endin

instr 2
  seed 1
  amin init 0
  kmax init 1
  aresult rspline amin, kmax, 100, 100
  aindex line 0, p3, p3 * sr
  areference table aindex, gireference
  kerror max_k abs(aresult - areference), 1, 1
  if kerror > .000000001 then
    printks "FAIL mixed-rate rspline differs by %g\n", 0, kerror
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
</CsScore>
</CsoundSynthesizer>
