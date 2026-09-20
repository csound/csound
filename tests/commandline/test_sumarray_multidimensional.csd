<CsTest>
description = "sumarray includes every element of a multidimensional array at all rates"
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
instr 1
  iInput[][] init 2, 3
  kInput[][] init 2, 3
  aInput[][] init 2, 3
  iRow = 0
  while iRow < 2 do
    iCol = 0
    while iCol < 3 do
      iInput[iRow][iCol] = iRow * 3 + iCol + 1
      iCol += 1
    od
    iRow += 1
  od
  iSum sumarray iInput
  if iSum != 21 then
    exitnow -1
  endif
  kInput[0][0] = 2
  kInput[1][2] = 3
  aInput[0][0] = 2
  aInput[1][2] = 3
  kSum sumarray kInput
  aSum sumarray aInput
  kAudio downsamp aSum
  if kSum != 5 || kAudio != 5 then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
e
</CsScore>
</CsoundSynthesizer>
