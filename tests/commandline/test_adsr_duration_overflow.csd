<CsTest>
description = "ADSR rejects a duration beyond the signed sample-count range"

[expect]
exit = "nonzero"
stderr = ["ADSR: duration is negative or out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  ; At kr = 512, this attack lasts 2^31 control cycles.
  ; Float represents that count exactly, but a signed 32-bit integer cannot.
  envelope:k adsr 4194304, .1, .5, .1
endin
</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
