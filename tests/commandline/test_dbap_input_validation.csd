<CsTest>
description = "reject unsafe DBAP layout and weight inputs"

[expect]
exit = "nonzero"
stderr = [
  "Need one weight per loudspeaker",
  "Invalid ftable no. 9999",
  "Dimension must be exactly 2",
  "9 errors in performance",
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
giShortWeights ftgen 0, 0, -1, -2, 1

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
  kGains dbapgains 0, kSource, giPositions, 0, 6, p4, iWeights
endin

instr 4
  kSource[] fillarray 0, 0
  kGains[] init 2
  kGains dbapgains 0, kSource, giPositions, 0, 6, 2, giShortWeights
endin

instr 5
  iPositions[][] init 2, 2
  iWeights[] fillarray 1
  kSource[] fillarray 0, 0
  aOut[] init 2
  aInput = 0
  aOut dbap aInput, 0, kSource, iPositions, 0, 6, iWeights
endin

instr 6
  iPositions[][] init 2, 2
  kSource[] fillarray 0, 0
  aOut[] init 2
  aInput = 0
  aOut dbap aInput, 0, kSource, iPositions, 0, 6, giShortWeights
endin

instr 7
  iWeights[] fillarray 1
  kSource[] fillarray 0, 0
  aOut[] init 2
  aInput = 0
  aOut dbap aInput, 0, kSource, giPositions, 0, 6, 2, iWeights
endin

instr 8
  kSource[] fillarray 0, 0
  aOut[] init 2
  aInput = 0
  aOut dbap aInput, 0, kSource, giPositions, 0, 6, 2.5
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 .01 .001
i 3 .02 .001 0
i 3 .03 .001 2.5
i 4 .04 .001
i 5 .05 .001
i 6 .06 .001
i 7 .07 .001
i 8 .08 .001
</CsScore>
</CsoundSynthesizer>
