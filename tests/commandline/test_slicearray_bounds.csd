<CsTest>
description = "slicearray handles large strides, in-place copies and local audio arrays"
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

opcode SliceAudio, k, a[]
  setksmps 4
  aInput[] xin
  aOutput[] slicearray aInput, 1, 2
  kFirst downsamp aOutput[0]
  kSecond downsamp aOutput[1]
  kValid = (kFirst == 2 && kSecond == 3 ? 1 : 0)
  xout kValid
endop

instr 1
  iInput[] init 129
  iInput[128] = 9
  ; The old loop overflowed on its final increment.
  iOutput[] slicearray iInput, 128, 128, 2147483520
  if lenarray(iOutput) != 1 || iOutput[0] != 9 then
    exitnow -1
  endif
  iAlias[] fillarray 1, 2, 3, 4
  iAlias slicearray iAlias, 1, 3, 2
  if lenarray(iAlias) != 2 || iAlias[0] != 2 || iAlias[1] != 4 then
    exitnow -1
  endif
  aInput[] init 3
  aInput[0] = 1
  aInput[1] = 2
  aInput[2] = 3
  kValid SliceAudio aInput
  if kValid != 1 then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
e
</CsScore>
</CsoundSynthesizer>
