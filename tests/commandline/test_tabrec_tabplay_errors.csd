<CsTest>
description = "tabrec and tabplay reject unusable tables and nonpositive tick counts"

[expect]
exit = "nonzero"
stderr = ["tabrec: table has no complete frame", "tabplay: table has no complete frame", "tabrec: tick count must be positive", "tabplay: tick count must be positive"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
giShort ftgen 1, 0, -2, -2, 0, 0
giValid ftgen 2, 0, -3, -2, 1, 2, 3
instr 1
  tabrec 1, 0, p5, p4, 1, 2
endin
instr 2
  kA init 0
  kB init 0
  tabplay 1, p5, p4, kA, kB
endin
</CsInstruments>
<CsScore>
i 1 0 .01 1 1
i 2 0 .01 1 1
i 1 0 .01 2 0
i 2 0 .01 2 0
i 1 0 .01 2 -1
i 2 0 .01 2 -1
</CsScore>
</CsoundSynthesizer>
