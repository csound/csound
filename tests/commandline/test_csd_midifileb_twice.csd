<CsTest>
description = "A second <CsMidifileB> replaces the first"
args = ["-nd"]

[expect]
exit = 0
output = ["MIDI note 62"]
output_excludes = ["MIDI note 60"]
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
</CsMidifileB>
<CsMidifileB>
TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPmRggD4AAP8vAA==
</CsMidifileB>
</CsoundSynthesizer>
