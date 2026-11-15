<CsTest>
description = "A <CsMidifileB> with no end tag stops Csound"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["Missing end tag </CsMidifileB>"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
<CsMidifileB>
TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPGRggDwAAP8vAA==
</CsoundSynthesizer>
