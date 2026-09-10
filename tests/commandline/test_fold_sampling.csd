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
  kcount init 0
  asignal = kcount + .25
  aresult fold asignal, p4
  kresult downsamp aresult

  if p4 == 1 then
    kexpected = kcount + .25
  elseif p4 == 2 then
    kexpected = int(kcount / 2) * 2 + .25
  else
    kindices[] fillarray 0, 0, 2, 3, 3, 5, 6, 6
    kexpected = kindices[kcount] + .25
  endif

  if !(abs(kresult - kexpected) < .000001) then
    printks "FAIL increment=%g sample=%g expected=%g actual=%g\n", \
            0, p4, kcount, kexpected, kresult
    exitnowk(-1)
  endif
  kcount += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .001 1
i 1 0 .001 2
i 1 0 .001 1.5
</CsScore>
</CsoundSynthesizer>
