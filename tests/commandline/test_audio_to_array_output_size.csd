<CsTest>
description = "Audio-to-array conversions restore the output length after trim"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
0dbfs = 32
gkChecks init 0

instr CheckOutputSize
  setksmps p4
  aInput = 3
  kConverted[] init ksmps
  kAssigned[] init ksmps
  ; Keep the allocated storage, but reduce the logical length every cycle.
  kShortLength = 1
  trim kConverted, kShortLength
  trim kAssigned, kShortLength
  kConverted = array(aInput)
  kAssigned = aInput
  kConvertedLength = lenarray(kConverted)
  kAssignedLength = lenarray(kAssigned)
  if kConvertedLength != ksmps || kAssignedLength != ksmps then
    printks "Expected %g elements, got array(a)=%g, assignment=%g\n", \
      0, ksmps, kConvertedLength, kAssignedLength
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < ksmps do
    if kConverted[kIndex] != 3 || kAssigned[kIndex] != 3 then
      printks "Expected sample 3 at index %g, got %g and %g\n", \
        0, kIndex, kConverted[kIndex], kAssigned[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 6 then
    prints "Expected six local control periods, checked %g\n", i(gkChecks)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Each note runs for two local control periods.
i "CheckOutputSize" 0 [16/32] 8
i "CheckOutputSize" .5 [8/32] 4
i "CheckOutputSize" 1 [2/32] 1
i "CheckResults" 1.5 .25
e
</CsScore>
</CsoundSynthesizer>
