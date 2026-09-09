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
  asignal = p4
  aresult distort1 asignal, 1000, 1, 0, 0, 2
  kresult downsamp aresult
  if !(abs(kresult - p4) < .000001) then
    printks "FAIL input=%g expected=%g actual=%g\n", 0, p4, p4, kresult
    exitnowk(-1)
  endif
endin

instr 2
  asignal = p4
  aresult distort1 asignal, 2, .7, .1, -.2, 2
  kresult downsamp aresult
  kexpected = ((exp(p4 * 2.2) - exp(p4 * -1.6)) /
              (exp(p4 * 2) + exp(p4 * -2))) * .7
  if !(abs(kresult - kexpected) < .000001) then
    printks "FAIL shaped input=%g expected=%g actual=%g\n", \
            0, p4, kexpected, kresult
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001 1
i 1 0 .001 -1
i 2 0 .001 -1
i 2 0 .001 -.5
i 2 0 .001 0
i 2 0 .001 .5
i 2 0 .001 1
</CsScore>
</CsoundSynthesizer>
