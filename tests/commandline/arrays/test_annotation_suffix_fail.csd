<CsTest>
description = "reject a malformed array annotation with suffix"

[expect]
exit = "nonzero"
stderr = ["syntax error"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  values:i[]x init 2
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
