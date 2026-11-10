<CsTest>
description = "a #loop outside a repeat stops the score with a message"

[expect]
exit = "nonzero"
stderr = ["#loop outside a repeat in score"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
endin
</CsInstruments>
<CsScore>
#loop
</CsScore>
</CsoundSynthesizer>
