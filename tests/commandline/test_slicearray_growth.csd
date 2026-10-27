<CsTest>
description = "slicearray follows changed global bounds and a growing source"
[expect]
exit = 0
output = ["warning: realloc: slicearray", "aSlice[] init 3", "kSlice[] init 3", "SSlice[] init 3"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m4
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1

giLast init 0

instr Slice
  ; Each output starts with one element. The source has room for four.
  aInput[] init 4
  aInput[0] = 10
  aInput[1] = 20
  aInput[2] = 30
  kInput[] fillarray 10, 20, 30, 40
  SInput[] fillarray "one", "two", "three", "four"
  aSlice[] slicearray aInput, 0, giLast
  kSlice[] slicearray kInput, 0, giLast
  SSlice[] slicearray SInput, 0, giLast
  kAudioLength lenarray aSlice
  kControlLength lenarray kSlice
  kStringLength lenarray SSlice

  kCycle timeinstk
  kLastIndex init 2
  ; ChangeBound has run by this point. All three slices must have grown.
  if kCycle >= 4 then
    if kAudioLength != 3 || kControlLength != 3 || kStringLength != 3 then
      exitnowk -1
    endif
    kAudio downsamp aSlice[kLastIndex]
    kString strcmpk SSlice[kLastIndex], "three"
    if kAudio != 30 || kSlice[kLastIndex] != 30 || kString != 0 then
      exitnowk -1
    endif
  endif
endin

instr ChangeBound
  ; An i-rate global can change when another note initializes.
  giLast = 2
endin

instr FollowSourceLength
  kInput[] fillarray 10, 20, 30, 40
  trimi kInput, 2
  ; Omitting the end bound follows the source's current logical length.
  kSlice[] slicearray kInput, 0
  kLength lenarray kSlice
  trim kInput, 4
  kCycle timeinstk
  kLastIndex init 3
  if kCycle >= 2 then
    if kLength != 4 || kSlice[kLastIndex] != 40 then
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "Slice" 0 .125
i "ChangeBound" .03125 .015625
i "FollowSourceLength" .125 .0625
e
</CsScore>
</CsoundSynthesizer>
