<CsTest>
description = "An orchestra named in <CsOptions> stands in for <CsInstruments>"
args = ["-nd"]

[expect]
exit = 0
output = ["named orchestra ran"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-d csd_named_orchestra.orc
</CsOptions>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
