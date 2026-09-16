<CsTest>
description = "vtable1k rejects a short vector after a table change"

[expect]
exit = "nonzero"
stderr = ["vtable1k: table is too short"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
giValid ftgen 0, 0, -3, -2, 1, 2, 3
giShort ftgen 0, 0, -2, -2, 1, 2
instr 1
  kTable init giValid
  kA init 0
  kB init 0
  kC init 0
  vtable1k kTable, kA, kB, kC
  kTable = giShort
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
