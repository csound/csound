<CsTest>
description = "unschedule requires instrument, start time and duration"
[expect]
exit = "nonzero"
stderr = ["unschedule: invalid argument count"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  unschedule 3
endin
instr 2
  unscheduleall 3, 0
endin
instr 3
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
