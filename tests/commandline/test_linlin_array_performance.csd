<CsTest>
description = "linlin array mapping, shorter blends and in-place output at performance time"
[expect]
exit = 0
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

instr Map
  ; Map [0, 0.5, 1] from [0, 1] to [10, 20].
  kInput[] fillarray 0, 0.5, 1
  kOutput[] linlin kInput, 10, 20
  if lenarray(kOutput) != 3 || kOutput[0] != 10 || kOutput[1] != 15 || kOutput[2] != 20 then
    printks "mapping returned the wrong values\n", 0
    exitnowk -1
  endif

  ; The same operation must also work when it overwrites its input.
  kInput linlin kInput, 10, 20
  if lenarray(kInput) != 3 || kInput[0] != 10 || kInput[1] != 15 || kInput[2] != 20 then
    printks "in-place mapping returned the wrong values\n", 0
    exitnowk -1
  endif
endin

instr Blend
  kInput[] fillarray 0, 0.5, 1
  kOther[] fillarray 10, 20, 30, 40
  ; Halfway between each pair: [5, 10.25, 15.5].
  ; Stop at the shorter array; the fourth value has no matching input.
  kOutput[] linlin 0.5, kInput, kOther
  if lenarray(kOutput) != 3 || kOutput[0] != 5 || kOutput[1] != 10.25 || kOutput[2] != 15.5 then
    printks "Blend returned the wrong values or length\n", 0
    exitnowk -1
  endif
endin

instr BlendIntoFirst
  kInput[] fillarray 0, 0.5, 1
  kOther[] fillarray 10, 20, 30, 40
  ; Halfway between each pair: [5, 10.25, 15.5].
  ; Stop at the shorter array; the fourth value has no matching input.
  kInput linlin 0.5, kInput, kOther
  if lenarray(kInput) != 3 || kInput[0] != 5 || kInput[1] != 10.25 || kInput[2] != 15.5 then
    printks "BlendIntoFirst returned the wrong values or length\n", 0
    exitnowk -1
  endif
endin

instr BlendIntoSecond
  kInput[] fillarray 0, 0.5, 1
  kOther[] fillarray 10, 20, 30, 40
  ; Halfway between each pair: [5, 10.25, 15.5].
  ; Stop at the shorter array; the fourth value has no matching input.
  kOther linlin 0.5, kInput, kOther
  if lenarray(kOther) != 3 || kOther[0] != 5 || kOther[1] != 10.25 || kOther[2] != 15.5 then
    printks "BlendIntoSecond returned the wrong values or length\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i "Map" 0 .015625
i "Blend" 0 .015625
i "BlendIntoFirst" 0 .015625
i "BlendIntoSecond" 0 .015625
e
</CsScore>
</CsoundSynthesizer>
