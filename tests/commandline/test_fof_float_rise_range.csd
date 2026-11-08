<CsTest>
description = "reject fof rise beyond the float-phase range"

[expect]
exit = "nonzero"
stderr = ["FOF rise time exceeds supported range"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

giTable ftgen 1, 0, 100, 7, 1, 100, 1

instr 1
  ; This rise exceeds the float-phase implementation's sample range.
  aOut fof 1, 0, 100, 0, 0, 50000, 0.00025, 0, 1, giTable, giTable, p3
  out aOut
endin
</CsInstruments>
<CsScore>
i1 0 0.001
e
</CsScore>
</CsoundSynthesizer>
