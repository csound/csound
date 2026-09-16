<CsTest>
description = "strtolk reports a range error when its string changes at performance time"
[expect]
exit = "nonzero"
stderr = ["strtolk: integer out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  SValue init "42"
  kCycle timeinstk
  if kCycle == 2 then
    SValue strcpyk "-2147483649"
  endif
  kValue strtolk SValue
endin
</CsInstruments>
<CsScore>
i1 0 .01
</CsScore>
</CsoundSynthesizer>
