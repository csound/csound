<CsTest>
description = "a comment the score does not close stops it with a message"

[expect]
exit = "nonzero"
stderr = ["unexpected EOF!!"]
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
i 1 0 1 /* never closed
</CsScore>
</CsoundSynthesizer>
