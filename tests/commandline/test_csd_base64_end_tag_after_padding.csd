<CsTest>
description = "A base64 section's end tag can follow padded data on its line"
args = ["-nd"]

[expect]
exit = 0
output = ["MIDI note 60", "file 1"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "MIDI note %d\n", notnum()
endin

instr 2
  prints "file %d\n", filevalid("csd_padded_end_tag.txt")
endin

</CsInstruments>
<CsScore>
i 2 0 0
f 0 0.5
</CsScore>
<CsMidifileB>
TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPGRggDwAAP8vAA==</CsMidifileB>
<CsFileB filename=csd_padded_end_tag.txt>
aGk=</CsFileB>
</CsoundSynthesizer>
