<CsTest>
description = "linlin follows changing array lengths within reserved output space"
[expect]
exit = 0
output = ["All three lengths checked"]
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

instr 1
  kInput[] fillarray 0, 0.5, 1
  kOther[] fillarray 10, 20, 30
  ; Reserve three slots, but begin with one visible input value.
  trim_i kInput, 1
  trim_i kOther, 1
  kMapped[] init 3
  kBlended[] init 3
  kCycle init 0
  kCycle += 1

  kMapped linlin kInput, 10, 20
  kBlended linlin 0.5, kInput, kOther
  ; Expected values stay the same; only the visible length changes.
  kExpectedMap[] fillarray 10, 15, 20
  kExpectedBlend[] fillarray 5, 10.25, 15.5
  kExpectedLength = (kCycle == 1 ? 1 : (kCycle == 2 ? 3 : 0))
  kMappedLength lenarray kMapped
  kBlendedLength lenarray kBlended
  if kMappedLength != kExpectedLength || kBlendedLength != kExpectedLength then
    printks "wrong output length on cycle %d\n", 0, kCycle
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kExpectedLength do
    if kMapped[kIndex] != kExpectedMap[kIndex] || kBlended[kIndex] != kExpectedBlend[kIndex] then
      printks "wrong value at index %d on cycle %d\n", 0, kIndex, kCycle
      exitnowk -1
    endif
    kIndex += 1
  od

  if kCycle == 1 then
    ; Grow for cycle two without allocating more memory.
    trim kInput, 3
    trim kOther, 3
  elseif kCycle == 2 then
    ; Empty inputs must produce empty outputs on cycle three.
    trim kInput, 0
    trim kOther, 0
  else
    printks "All three lengths checked\n", 0
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .046875
e
</CsScore>
</CsoundSynthesizer>
