<CsTest>
description = "<CsMidifileB> also ends at </CsMidiFileB>, the end tag it once asked for"
args = ["-nd"]

[expect]
exit = 0
output = ["MIDI note 60"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "MIDI note %d\n", notnum()
endin

</CsInstruments>
<CsScore>
f 0 0.5
</CsScore>
<CsMidifileB>
TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPGRggDwAAP8vAA==
</CsMidiFileB>
</CsoundSynthesizer>
