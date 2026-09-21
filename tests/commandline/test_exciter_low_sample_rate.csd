<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 22050
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  aIn oscili .1, 1500
  aOut exciter aIn, 1000, 10000, 1, 0
  kOut downsamp aOut
  ; The resampling filters must stay stable with valid in-band cutoffs.
  if !(abs(kOut) < 1) then
    printks "exciter output grew without bound: %g\n", 0, kOut
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1
</CsScore>
</CsoundSynthesizer>
