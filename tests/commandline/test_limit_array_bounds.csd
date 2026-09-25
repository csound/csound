<CsTest>
description = "Array limit matches scalar clipping for ordered, equal and reversed bounds"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
gkChecks init 0

instr CheckInit
  iInput[] fillarray -4, -2, 0, 2, 4
  iClipped[] = limit(iInput, -2, 2)
  iReversed[] = limit(iInput, 4, -2)
  iEqual[] = limit(iInput, 2, 2)
  iReused[] = iInput
  iReused = limit(iReused, 4, -2)
  iIndex = 0
  while iIndex < lenarray(iInput) do
    iExpected = limit(iInput[iIndex], -2, 2)
    ; Reversed bounds always give (4 + -2)/2 = 1, regardless of input.
    if iClipped[iIndex] != iExpected || iReversed[iIndex] != 1 || \
       iEqual[iIndex] != 2 || iReused[iIndex] != 1 then
      prints "Init limit element %g: clipped=%g, reversed=%g, equal=%g, reused=%g\n", \
        iIndex, iClipped[iIndex], iReversed[iIndex], iEqual[iIndex], iReused[iIndex]
      exitnow -1
    endif
    iIndex += 1
  od
endin

instr CheckPerformance
  kInput[] fillarray -4, -2, 0, 2, 4
  kCycle timeinstk
  ; Change bounds without reinitializing the opcode.
  if kCycle == 1 then
    kLow = -2
    kHigh = 2
  elseif kCycle == 2 then
    kLow = 4
    kHigh = -2
  elseif kCycle == 3 then
    kLow = 2
    kHigh = 2
  else
    kLow = 4
    kHigh = -2
  endif
  ; Exercise reversed bounds with a shorter array and with no elements.
  if kCycle == 4 then
    trim kInput, 2
  elseif kCycle == 5 then
    trim kInput, 0
  endif
  kOutput[] = limit(kInput, kLow, kHigh)
  kReused[] = kInput
  kReused = limit(kReused, kLow, kHigh)
  kCount lenarray kInput
  kOutputCount lenarray kOutput
  kReusedCount lenarray kReused
  if kOutputCount != kCount || kReusedCount != kCount then
    printks "limit returned the wrong array length\n", 0
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kCount do
    kExpected = limit(kInput[kIndex], kLow, kHigh)
    if kOutput[kIndex] != kExpected || kReused[kIndex] != kExpected then
      printks "limit cycle %g element %g: expected %g, got %g (reused %g)\n", \
        0, kCycle, kIndex, kExpected, kOutput[kIndex], kReused[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  gkChecks += 1
  if kCycle == 5 then
    turnoff
  endif
endin

instr CheckResults
  if i(gkChecks) != 5 then
    prints "Array limit checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckInit" 0 .001
i "CheckPerformance" 0 .003
i "CheckResults" .004 .001
e
</CsScore>
</CsoundSynthesizer>
