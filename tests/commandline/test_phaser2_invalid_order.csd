<CsTest>
description = "phaser2 rejects invalid stage counts before allocating state"
[expect]
exit = "nonzero"
stderr = ["phaser2: invalid stage count"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
instr 1
  iOrder = (p4 == -2 ? sqrt:i(-1) : p4)
  aInput = 1
  aOutput phaser2 aInput, 1000, .5, iOrder, 1, 1, 0
endin
</CsInstruments>
<CsScore>
i 1 0 .01 -1
i 1 0 .01 2147483648
i 1 0 .01 1e30
i 1 0 .01 -2
e
</CsScore>
</CsoundSynthesizer>
