<CsTest>
description = "fail when struct init provides only some members"

[expect]
exit = "nonzero"
stderr = [":Point; init c"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

struct Point x:i, y:i

instr 1
  point:Point init 7
endin

</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
