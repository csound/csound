<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  aresult rspline 0, 1, 1000000000000, 1000000000000
  kresult downsamp aresult
  if !(kresult == kresult) then
    printks "rspline returned NaN for a large finite rate\n", 0
    exitnowk -1
  endif
endin

instr 2
  kresult rspline 0, 1, 1000000000000, 1000000000000
  if !(kresult == kresult) then
    printks "control rspline returned NaN for a large finite rate\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 0 .001
</CsScore>
</CsoundSynthesizer>
