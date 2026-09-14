<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

gireference ftgen 0, 0, 1024, -2, 0

instr 1
  seed 1
  kresult rspline 0, 1, 100, 100
  kindex init 0
  tablew kresult, kindex, gireference
  kindex += 1
endin

instr 2
  seed 1
  aresult rspline 0, 1, 100, 100
  kindex init 0
  kresult downsamp aresult
  kreference table kindex, gireference
  if abs(kresult - kreference) > .000000001 then
    printks "FAIL sample=%g control=%g audio=%g\n", \
            0, kindex, kreference, kresult
    exitnowk -1
  endif
  kindex += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
</CsScore>
</CsoundSynthesizer>
