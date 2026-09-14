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
  kenv envlpx 1, 1, 1, .1, 1, 1, .1
  kcount init 0
  if kcount == 1 then
    if abs(kenv - 0.0000208333) > 0.00001 then
      printks "envlpx second sample mismatch: expected about .000021, got %f\\n", 1, kenv
      exitnowk(-1)
    endif
  endif
  kcount += 1
endin
</CsInstruments>
<CsScore>
f 1 0 -100 -7 0 100 1
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
