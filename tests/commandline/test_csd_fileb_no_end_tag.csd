<CsTest>
description = "A <CsFileB> with no end tag stops Csound"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["Missing end tag </CsFileB>"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
<CsFileB filename=csd_fileb_no_end_tag.mid>
TVRoZAAAAAYAAAABAGBNVHJrAAAADACQPGRggDwAAP8vAA==
</CsoundSynthesizer>
