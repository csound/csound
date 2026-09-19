<CsTest>
description = "linlin rejects matrix inputs before changing output at either rate"
[expect]
exit = "nonzero"
stderr = ["INIT ERROR", "linlin: expected one-dimensional arrays"]
output = [
  "InitMap: output unchanged",
  "InitBlendFirst: output unchanged",
  "InitBlendSecond: output unchanged",
  "ControlMap: output unchanged",
  "ControlBlendFirst: output unchanged",
  "ControlBlendSecond: output unchanged"
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giOutput[] fillarray 7, 8, 9
gkOutput[] fillarray 7, 8, 9
instr InitMap
  ; A matrix is invalid even though it contains numeric values.
  iInput[][] init 2, 2
  giOutput linlin iInput, 10, 20
endin

instr InitBlendFirst
  ; A matrix is invalid even though it contains numeric values.
  iFirst[][] init 2, 2
  iSecond[] init 3
  giOutput linlin 0.5, iFirst, iSecond
endin

instr InitBlendSecond
  ; A matrix is invalid even though it contains numeric values.
  iFirst[] init 3
  iSecond[][] init 2, 2
  giOutput linlin 0.5, iFirst, iSecond
endin

instr ControlMap
  ; A matrix is invalid even though it contains numeric values.
  kInput[][] init 2, 2
  gkOutput linlin kInput, 10, 20
endin

instr ControlBlendFirst
  ; A matrix is invalid even though it contains numeric values.
  kFirst[][] init 2, 2
  kSecond[] init 3
  gkOutput linlin 0.5, kFirst, kSecond
endin

instr ControlBlendSecond
  ; A matrix is invalid even though it contains numeric values.
  kFirst[] init 3
  kSecond[][] init 2, 2
  gkOutput linlin 0.5, kFirst, kSecond
endin

instr CheckOutput
  ; An error must leave both the values and the output length unchanged.
  if lenarray(giOutput) == 3 && \
      giOutput[0] == 7 && \
      giOutput[1] == 8 && \
      giOutput[2] == 9 && \
      lenarray(gkOutput) == 3 && \
      gkOutput[0] == 7 && \
      gkOutput[1] == 8 && \
      gkOutput[2] == 9 then
    SCase strget p4
    printf "%s: output unchanged\n", 1, SCase
  endif
endin
</CsInstruments>
<CsScore>
i "InitMap" 0 .015625
i "CheckOutput" 0.03125 .015625 "InitMap"
i "InitBlendFirst" 0.0625 .015625
i "CheckOutput" 0.09375 .015625 "InitBlendFirst"
i "InitBlendSecond" 0.125 .015625
i "CheckOutput" 0.15625 .015625 "InitBlendSecond"
i "ControlMap" 0.1875 .015625
i "CheckOutput" 0.21875 .015625 "ControlMap"
i "ControlBlendFirst" 0.25 .015625
i "CheckOutput" 0.28125 .015625 "ControlBlendFirst"
i "ControlBlendSecond" 0.3125 .015625
i "CheckOutput" 0.34375 .015625 "ControlBlendSecond"
e
</CsScore>
</CsoundSynthesizer>
