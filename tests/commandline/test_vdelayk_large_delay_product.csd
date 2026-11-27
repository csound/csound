<CsTest>
description = "vdelayk wraps the exact sample offset for a large delay"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1000
ksmps=1
nchnls=1
0dbfs=1

instr 1
  kSample init 0
  kOut vdelayk kSample+1, 16777215, 7/sr, 0, 1
  if kSample == 7 then
    if kOut != 8 then
      printks "vdelayk lost whole sample bits before wrapping\n", 0
      exitnowk -1
    endif
    turnoff
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008
</CsScore>
</CsoundSynthesizer>
