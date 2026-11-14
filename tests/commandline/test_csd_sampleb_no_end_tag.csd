<CsTest>
description = "A <CsSampleB> with no end tag stops Csound"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["Missing end tag </CsSampleB>"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
<CsSampleB filename="9731">
TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPGRggDwAAP8vAA==
</CsoundSynthesizer>
