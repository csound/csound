<CsTest>
description = "Reject MIDI file ports outside the supported range"

[expect]
exit = "nonzero"
stderr = ["midifileopen: port out of range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iFile midifileopen "catherine.mid", p4
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 0 .01 64
i 1 0 .01 1e20
</CsScore>
</CsoundSynthesizer>
