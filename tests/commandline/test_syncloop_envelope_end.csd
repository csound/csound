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
  ; Four-sample grains start every four samples. Each old grain must retire
  ; before its overlap slot is reused, keeping the output at one.
  aout syncloop 1, 2000, 1, 0.0005, 0, 0, 0.001, 1, 2, 1
  kout downsamp aout
  if !(abs(kout - 1) <= 0.001) then
    printks "syncloop envelope-end mismatch: actual=%.9f expected=1\n", 0, kout
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
f1 0 8 -2 1 1 1 1 1 1 1 1
f2 0 4 -2 1 1 1 1
i1 0 0.003
</CsScore>
</CsoundSynthesizer>
