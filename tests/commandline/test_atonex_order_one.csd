<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  asig mpulse 1, 0
  areference atone asig, 1000
  aresult atonex asig, 1000, 1
  kerror max_k abs(aresult - areference), 1, 1
  if !(kerror < .000001) then
    printks "atonex order one mismatch: %g\n", 0, kerror
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02
</CsScore>
</CsoundSynthesizer>
