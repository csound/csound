<CsTest>
description = "mkir.csd"
skip = "Manual impulse-response workflow: requires generated sweep.wav and rev.wav."

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
 -U mkir sweep.wav -i rev.wav -o ir.wav
</CsOptions>
<CsInstruments>
</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>
