<CsTest>
description = "linlin array mapping, shorter blends and in-place output at init time"
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
  iInput[] fillarray 0, 0.5, 1
  iOutput[] linlin iInput, 10, 20
  if lenarray(iOutput) != 3 || iOutput[0] != 10 || iOutput[1] != 15 || iOutput[2] != 20 then
    prints "mapping returned the wrong values\n"
    exitnow -1
  endif

  ; The same operation must also work when it overwrites its input.
  iInput linlin iInput, 10, 20
  if lenarray(iInput) != 3 || iInput[0] != 10 || iInput[1] != 15 || iInput[2] != 20 then
    prints "in-place mapping returned the wrong values\n"
    exitnow -1
  endif
endin

instr Blend
  iInput[] fillarray 0, 0.5, 1
  iOther[] fillarray 10, 20, 30, 40
  ; Halfway between each pair: [5, 10.25, 15.5].
  ; Stop at the shorter array; the fourth value has no matching input.
  iOutput[] linlin 0.5, iInput, iOther
  if lenarray(iOutput) != 3 || iOutput[0] != 5 || iOutput[1] != 10.25 || iOutput[2] != 15.5 then
    prints "Blend returned the wrong values or length\n"
    exitnow -1
  endif
endin

instr BlendIntoFirst
  iInput[] fillarray 0, 0.5, 1
  iOther[] fillarray 10, 20, 30, 40
  ; Halfway between each pair: [5, 10.25, 15.5].
  ; Stop at the shorter array; the fourth value has no matching input.
  iInput linlin 0.5, iInput, iOther
  if lenarray(iInput) != 3 || iInput[0] != 5 || iInput[1] != 10.25 || iInput[2] != 15.5 then
    prints "BlendIntoFirst returned the wrong values or length\n"
    exitnow -1
  endif
endin

instr BlendIntoSecond
  iInput[] fillarray 0, 0.5, 1
  iOther[] fillarray 10, 20, 30, 40
  ; Halfway between each pair: [5, 10.25, 15.5].
  ; Stop at the shorter array; the fourth value has no matching input.
  iOther linlin 0.5, iInput, iOther
  if lenarray(iOther) != 3 || iOther[0] != 5 || iOther[1] != 10.25 || iOther[2] != 15.5 then
    prints "BlendIntoSecond returned the wrong values or length\n"
    exitnow -1
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
