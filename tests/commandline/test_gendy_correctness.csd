<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 64
nchnls = 1
0dbfs = 1

giBlock ftgen 0, 0, 64, -2, 0
giSingle ftgen 0, 0, 64, -2, 0

instr 1
  seed 1
  aresult gendyc .5, 1, 1, 1, 1, 4000, 4000, .5, .5, 12, 12
  aindex line 0, p3, p3 * sr
  tablew aresult, aindex, giBlock
endin

instr 2
  setksmps 1
  seed 1
  aresult gendyc .5, 1, 1, 1, 1, 4000, 4000, .5, .5, 12, 12
  aindex line 0, p3, p3 * sr
  tablew aresult, aindex, giSingle
endin

instr 3
  iindex = 0
  while iindex < 64 do
    iblock table iindex, giBlock
    isingle table iindex, giSingle
    if abs(iblock - isingle) > .000000001 then
      prints "gendyc differs at sample %d: block %.12g, single %.12g\n", \
             iindex, iblock, isingle
      exitnow -1
    endif
    iindex += 1
  od
endin

instr 4
  seed 1
  aresult gendyc .5, 1, 1, 1, 1, .00001, .00001, .5, .5, 12, 12
  out aresult
endin

instr 5
  iNaN strtod "nan"
  seed 1
  aresult gendy .5, iNaN, iNaN, .5, .5, 100, 100, .5, .5, iNaN, iNaN
  kresult downsamp aresult
  if qnan(kresult) != 0 then
    printks "gendy produced a non-finite value for defaulted selectors\n", 0
    exitnowk -1
  endif
  out aresult
endin

instr 6
  seed 1
  agendy gendy .5, 1, 1, .5, .5, 8000, 8000, .5, .5, 12, 12
  agendyx gendyx .5, 1, 1, .5, .5, 8000, 8000, .5, .5, 1, 1, 12, 12
  kgendy max_k abs(agendy), 1, 1
  kgendyx max_k abs(agendyx), 1, 1
  if kgendy > .500000001 || kgendyx > .500000001 then
    printks "high-frequency Gendy output exceeded its amplitude: %g, %g\n", \
            0, kgendy, kgendyx
    exitnowk -1
  endif
endin

instr 7
  kcurve init -1
  aresult gendyx .5, 1, 1, .5, .5, 100, 100, .5, .5, \
                 kcurve, kcurve, 12, 12
  if kcurve != -1 then
    printks "gendyx changed its curve input to %g\n", 0, kcurve
    exitnowk -1
  endif
  out aresult
endin
</CsInstruments>
<CsScore>
i 1 0 .008
i 2 .01 .008
i 3 .02 .001
i 4 .03 .001
i 5 .04 .008
i 6 .05 .008
i 7 .06 .008
</CsScore>
</CsoundSynthesizer>
