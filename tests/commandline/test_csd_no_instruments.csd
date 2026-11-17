<CsTest>
description = "A CSD with no <CsInstruments> says so, and is not compiled as an orchestra"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["has no <CsInstruments>"]
</CsTest>
<CsoundSynthesizer>
<CsScore>
e
</CsScore>
</CsoundSynthesizer>
