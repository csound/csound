<CsTest>
description = "reject unsafe DBAP layout and weight inputs"

[expect]
exit = "nonzero"
stderr = [
  "Need one weight per loudspeaker",
  "Invalid ftable no. 9999",
  "Dimension must be exactly 2",
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1

giPositions ftgen 0, 0, 4, -2, -1, 0, 1, 0

instr 1
  iPositions[][] init 3, 2
  iWeights[] fillarray 1
  kSource[] fillarray 0, 0
  kGains[] init 3
  kGains dbapgains 0, kSource, iPositions, 0, 6, iWeights
endin

instr 2
  iPositions[][] init 2, 2
  kSource[] fillarray 0, 0
  kGains[] init 2
  kGains dbapgains 0, kSource, iPositions, 0, 6, 9999
endin

instr 3
  iWeights[] fillarray 1, 1
  kSource[] fillarray 0, 0
  kGains[] init 2
  kGains dbapgains 0, kSource, giPositions, 0, 6, 0, iWeights
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 .01 .001
i 3 .02 .001
</CsScore>
</CsoundSynthesizer>
