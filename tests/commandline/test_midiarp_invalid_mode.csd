<CsTest>
description = "midiarp rejects unsupported modes"

[expect]
exit = "nonzero"
stderr = ["midiarp: mode must be 0, 1, 2, or 3"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  kNote, kTrigger midiarp 10, 4
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
