<CsoundSynthesizer>
<CsOptions>
-n -d  -j 2
</CsOptions>
<CsInstruments>
sr = 64
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  kcnt init 0
  setksmps 4
  kcnt += 1
  kwhen timeinstk
  if kwhen != kcnt then
   exitnowk(1)
  endif
  printks "kcounter: %d, kcnt: %d\n", 0, kwhen, kcnt
endin

</CsInstruments>
<CsScore>
i 1 0 0.25
e
</CsScore>
</CsoundSynthesizer>