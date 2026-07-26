<CsoundSynthesizer>
<CsOptions>
-n -d --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  SNames[] fillarray "left", "right"
  aBase phasor 1
  aValues[] init 2
  aValues[0] = aBase
  aValues[1] = aBase + 100
  chnseta aValues, SNames
endin

instr 2
  setksmps 4
  SNames[] fillarray "left", "right"
  aValues[] chngeta SNames
  kDifference = k(aValues[1]) - k(aValues[0])
  if kDifference != 100 then
    printks "aligned read mismatch: %.6f\n", 0, kDifference
    exitnowk(-1)
  endif
endin

instr 3
  setksmps 4
  SNames[] fillarray "left", "right"
  aValues[] chngeta SNames
  kFirst init 1
  kOffset offsetsmps
  if kFirst == 1 then
    if kOffset == 0 || k(aValues[1]) - k(aValues[0]) != 100 then
      printks "offset read mismatch: offset=%d, difference=%.6f\n", 0, \
              kOffset, k(aValues[1]) - k(aValues[0])
      exitnowk(-1)
    endif
    kFirst = 0
  elseif k(aValues[1]) - k(aValues[0]) != 100 then
    printks "offset read channels are out of step: %.6f\n", 0, \
            k(aValues[1]) - k(aValues[0])
    exitnowk(-1)
  endif
endin

instr 4  ; writer with a local ksmps
  setksmps 4
  SNames[] fillarray "wleft", "wright"
  aBase phasor 1
  aValues[] init 2
  aValues[0] = aBase
  aValues[1] = aBase + 100
  chnseta aValues, SNames
endin

instr 5  ; reader at the global ksmps
  SNames[] fillarray "wleft", "wright"
  aValues[] chngeta SNames
  kStarted init 0
  if kStarted == 1 && k(aValues[1]) - k(aValues[0]) != 100 then
    printks "local-ksmps write mismatch: %.6f\n", 0, \
            k(aValues[1]) - k(aValues[0])
    exitnowk(-1)
  endif
  kStarted = 1
endin
</CsInstruments>
<CsScore>
i 1 0 0.5
i 2 0 0.5
i 3 0.03125 0.25
i 4 0 0.5
i 5 0 0.5
</CsScore>
</CsoundSynthesizer>
