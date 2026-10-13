<CsTest>
description = "reshape follows control, audio, string, and struct values during performance"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

struct Sample value:k

instr 1
  kCycle timeinstk
  kSource[] init 6
  kSource[5] = kCycle
  kMatrix[][] reshapearray kSource, 2, 3
  if kMatrix[1][2] != kCycle || lenarray(kSource, 0) != 1 then
    printks "control reshape lost a value or changed source rank\n", 0
    exitnowk(-1)
  endif
  kMatrix[1][2] = -1
  if kSource[5] != kCycle then
    printks "control reshape still aliases its source\n", 0
    exitnowk(-1)
  endif

  aSource[] init 6
  aSource[5] = 10*kCycle
  aMatrix[][] reshapearray aSource, 2, 3
  kIndex = 0
  while kIndex < ksmps do
    kActual vaget kIndex, aMatrix[1][2]
    if kActual != 10*kCycle then
      printks "audio reshape lost a sample\n", 0
      exitnowk(-1)
    endif
    kIndex += 1
  od

  SSource[] fillarray "first", "second"
  SSource[1] = sprintfk("cycle %d", kCycle)
  SMatrix[][] reshapearray SSource, 1, 2
  kDifferent strcmpk SMatrix[0][1], SSource[1]
  if kDifferent != 0 then
    printks "string reshape lost a value\n", 0
    exitnowk(-1)
  endif

  source:Sample[] init 2
  current:Sample init 0
  source[1] = current
  current.value = kCycle
  kLast init 1
  source[kLast] = current
  matrix:Sample[][] reshapearray source, 1, 2
  if matrix[0][kLast].value != kCycle then
    printks "struct reshape lost a control value\n", 0
    exitnowk(-1)
  endif
  replacement:Sample init -1
  matrix[0][kLast] = replacement
  if source[kLast].value != kCycle then
    printks "struct reshape still aliases its source\n", 0
    exitnowk(-1)
  endif

endin
</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
