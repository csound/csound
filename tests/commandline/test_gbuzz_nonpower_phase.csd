<CsoundSynthesizer>
<CsOptions>
-d -m0 -n
</CsOptions>
<CsInstruments>
sr=48000
ksmps=1
nchnls=1
0dbfs=1

instr 1
  asig gbuzz .01, 480, 3, 1, 0.5, 1
  kval downsamp asig
  kcount init 0
  if kcount == 0 then
    if abs(kval - 0.01) > 0.0001 then
      printks "gbuzz first sample mismatch: expected .01, got %f\\n", 1, kval
      exitnowk(-1)
    endif
  endif
  kcount += 1
endin
</CsInstruments>
<CsScore>
f 1 0 100 11 1
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
