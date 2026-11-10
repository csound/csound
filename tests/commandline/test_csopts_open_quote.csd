<CsTest>
description = "A quote left open ends at the line's end, not in an earlier line's text"
args = ["-nd"]

[expect]
exit = 0
output = ["ksmps 32"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
; xxxxxxxx --ksmps=16
-o "x
</CsOptions>
<CsInstruments>
ksmps = 32

instr 1
  prints "ksmps %d\n", ksmps
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
