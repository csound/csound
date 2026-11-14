<CsTest>
description = "A CSD with no <CsInstruments> still lists the opcodes its <CsOptions> ask for"
args = ["-nd"]

[expect]
exit = 0
output = ["oscil"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-z
</CsOptions>
</CsoundSynthesizer>
