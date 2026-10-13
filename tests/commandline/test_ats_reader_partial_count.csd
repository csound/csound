<CsTest>
description = "ATSpartialtap checks the partial count when its active reader changes"
[expect]
exit = "nonzero"
stderr = ["ATSPARTIALTAP: exceeded max partial 1", "1 errors in performance"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  kCycle init 0
  ; Both branches initialize. The last one supplies four partials at init.
  ; On the second cycle, the reader supplies only its first partial.
  if kCycle == 1 then
    atsbufread 0, 1, "ats-instance.ats", 1
  else
    atsbufread 0, 1, "ats-instance.ats", 4
  endif
  kFreq, kAmp atspartialtap 2
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
