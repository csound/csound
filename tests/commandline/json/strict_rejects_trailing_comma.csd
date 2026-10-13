<CsTest>
description = "JSON rejects a trailing comma unless the caller enables it"

[expect]
exit = "nonzero"
stderr = ["jsonunmarshal: trailing comma is not allowed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
struct Note pitch:i

instr 1
  note:Note jsonunmarshal {{ {"pitch": 64,} }}
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
