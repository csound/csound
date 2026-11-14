<CsTest>
description = "A CSD that fails to read is not given an unrelated system error"
args = ["-nd"]

[expect]
exit = "nonzero"
stderr = ["Missing end tag </CsInstruments>"]
stderr_regex = ['Reading CSD failed\.\.\. stopping']
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
endin

