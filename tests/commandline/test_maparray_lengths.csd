<CsTest>
description = "maparray maps the current input length regardless of output preallocation"
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
  iInput[] fillarray 4, 9
  iOutput[] init 1
  iOutput maparray iInput, "sqrt"
  if lenarray(iOutput) != 2 || iOutput[0] != 2 || iOutput[1] != 3 then
    exitnow -1
  endif
  kInput[] fillarray 4, 9, 16
  trim_i kInput, 2
  kOutput[] init 4
  kOutput maparray kInput, "sqrt"
  kLength lenarray kInput
  kOutLength lenarray kOutput
  if kOutLength != kLength then
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    if kOutput[kIndex] != kIndex + 2 then
      exitnowk -1
    endif
    kIndex += 1
  od
  kCycle timeinstk
  kNext = (kCycle == 1 ? 3 : (kCycle == 2 ? 1 : 0))
  trim kInput, kNext
endin
</CsInstruments>
<CsScore>
i 1 0 .0625
e
</CsScore>
</CsoundSynthesizer>
