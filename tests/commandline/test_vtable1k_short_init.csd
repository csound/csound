<CsTest>
description = "vtable1k rejects a short vector at initialization"

[expect]
exit = "nonzero"
stderr = ["vtable1k: table is too short"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
giShort ftgen 0, 0, -2, -2, 1, 2
instr 1
  kA init 0
  kB init 0
  kC init 0
  vtable1k giShort, kA, kB, kC
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
