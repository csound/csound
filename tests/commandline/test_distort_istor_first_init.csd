<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

gishape ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1

instr 1
  asignal init 0
  aresult distort asignal, 1, gishape, 10, 1
  kresult downsamp aresult
  iexpected = 1000 / 32768
  if !(abs(kresult - iexpected) < .000000001) then
    printks "FAIL expected=%g actual=%g\n", 0, iexpected, kresult
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
