<CsTest>
description = "gensweep.csd"
skip = "Manual impulse-response workflow: generates sweep.wav for mkrev and mkir."

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
 -U mkir -g sweep.wav 
</CsOptions>
<CsInstruments>
</CsInstruments>
<CsScore>
</CsScore>
</CsoundSynthesizer>
