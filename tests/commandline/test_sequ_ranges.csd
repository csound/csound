<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; Exercise each playback mode and check the permutation through sequstate.
instr 1
 iRhythm[] fillarray 1, 1, 1, 1, 1
 iInstruments[] fillarray 0, 0, 0, 0, 0
 iData[] fillarray 0, 0, 0, 0, 0
 kCycle init 0
 kCycle += 1
 kStep = (kCycle <= p7 ? -1 : 1)
 if p6 == 0 then
  kIndex sequ iRhythm, iInstruments, iData, 60, 5, p4, kStep
 else
  kIndex sequ p5, iRhythm, iInstruments, iData, 60, 5, p4, kStep
 endif
 kCount, kMap[] sequstate 0
 kEvent = kCycle-p7-1
 iWidth = 5-p5
 kExpected = -1
 if kEvent >= 0 then
  if p4 >= 0 || p4 == -6 then
   kExpected = p5 + kEvent % iWidth
  elseif p4 == -1 then
   kExpected = 4 - kEvent % iWidth
  elseif p4 == -2 then
   kPosition = kEvent % (2*iWidth)
   kExpected = (kPosition < iWidth ? p5+kPosition : 4-(kPosition-iWidth))
  elseif p4 == -3 then
   kExpected = kIndex
   if kIndex < p5 || kIndex >= 5 then
    printks "sequ random index outside range\n", 0
    exitnowk(-1)
   endif
  elseif p4 == -4 && kEvent < iWidth then
   kExpected = p5+kEvent
  elseif p4 == -5 && kEvent < iWidth then
   kExpected = 4-kEvent
  elseif p4 == -7 then
   kExpected = p5
  endif
 endif
 if kIndex != kExpected then
  printks "sequ mode=%g start=%g cycle=%g: index=%g expected=%g\n", 0, p4, p5, kCycle, kIndex, kExpected
  exitnowk(-1)
 endif
 kI = 0
 while kI < 5 do
  if kMap[kI] < 0 || kMap[kI] >= 5 || (kI < p5 && kMap[kI] != kI) then
   printks "sequ permutation escaped its range\n", 0
   exitnowk(-1)
  endif
  kJ = kI+1
  while kJ < 5 do
   if kMap[kI] == kMap[kJ] then
    printks "sequ permutation contains a duplicate\n", 0
    exitnowk(-1)
   endif
   kJ += 1
  od
  kI += 1
 od
 if kCycle == 32 then
  gkChecks += 1
 endif
endin

; After mutation, timing follows the selected note, not its former slot.
instr 2
 iRhythm[] fillarray 1/512, 3/512, 5/512
 iInstruments[] fillarray 90, 90, 90
 iData[] fillarray 1/512, 3/512, 5/512
 kPreviousMap[] fillarray 0, 1, 2
 kCycle init 0
 kDue init 1
 kCycle += 1
 if p4 == 0 then
  kIndex sequ iRhythm, iInstruments, iData, 60, 3, 1
 else
  kIndex sequ 1, iRhythm, iInstruments, iData, 60, 3, 1
 endif
 kCount, kMap[] sequstate 0
 if (kCycle == kDue && kIndex < 0) || (kCycle != kDue && kIndex >= 0) then
  printks "sequ timing: cycle=%g due=%g index=%g\n", 0, kCycle, kDue, kIndex
  exitnowk(-1)
 endif
 if kIndex >= 0 then
  kSelected = kPreviousMap[kIndex]
  kDue = kCycle + iRhythm[kSelected]*512
 endif
 kPreviousMap = kMap
 if kCycle == 64 then
  gkChecks += 1
 endif
endin

; Changing a range and playback direction must not leave a stale index.
instr 3
 iRhythm[] fillarray 1, 1, 1, 1, 1
 iInstruments[] fillarray 0, 0, 0, 0, 0
 iData[] fillarray 0, 0, 0, 0, 0
 kCycle init 0
 kNext init 0
 kCycle += 1
 kStart = (kCycle < 6 ? 0 : (kCycle < 13 ? 3 : 2))
 kLength = (kCycle < 13 ? 5 : 4)
 kMode = (kCycle >= 17 && kCycle < 23 ? -1 : 0)
 kIndex sequ kStart, iRhythm, iInstruments, iData, 60, kLength, kMode, 1
 if kNext < kStart || kNext >= kLength then
  kNext = (kMode == 0 ? kStart : kLength-1)
 endif
 if kIndex != kNext then
  printks "sequ range change: cycle=%g index=%g expected=%g\n", 0, kCycle, kIndex, kNext
  exitnowk(-1)
 endif
 kNext += (kMode == 0 ? 1 : -1)
 if kCycle == 32 then
  gkChecks += 1
 endif
endin

; The documented maximum of 128 steps is usable.
instr 4
 iRhythm[] init 128
 iInstruments[] init 128
 iData[] init 128
 kIndex sequ iRhythm, iInstruments, iData, 60, 128, 0, 1
 kCycle init 0
 if kIndex != kCycle then
  printks "sequ 128-step index mismatch\n", 0
  exitnowk(-1)
 endif
 kCycle += 1
 if kCycle == 128 then
  gkChecks += 1
 endif
endin
instr 90
 if abs(p3-p4) > .000001 then
  prints "sequ event duration disagrees with selected note\n"
  exitnow(-1)
 endif
endin
instr 99
 if i(gkChecks) != 26 then
  prints "sequ checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0625 0 0 0 0
i 1 0.125 .0625 -1 0 0 0
i 1 0.25 .0625 -2 0 0 0
i 1 0.375 .0625 -3 0 0 0
i 1 0.5 .0625 -4 0 0 0
i 1 0.625 .0625 -5 0 0 0
i 1 0.75 .0625 -6 0 0 0
i 1 0.875 .0625 -7 0 0 0
i 1 1 .0625 -8 0 0 0
i 1 1.125 .0625 1 0 0 0
i 1 1.25 .0625 0 3 1 0
i 1 1.375 .0625 -1 3 1 0
i 1 1.5 .0625 -2 3 1 0
i 1 1.625 .0625 -3 3 1 0
i 1 1.75 .0625 -4 3 1 0
i 1 1.875 .0625 -5 3 1 0
i 1 2 .0625 -6 3 1 0
i 1 2.125 .0625 -7 3 1 0
i 1 2.25 .0625 -8 3 1 0
i 1 2.375 .0625 1 3 1 0
i 1 2.5 .0625 0 3 1 3
i 1 2.625 .0625 -2 3 1 3
i 2 2.75 .125 0
i 2 3 .125 1
i 3 3.25 .0625
i 4 3.375 .25
i 99 3.75 .002
e
</CsScore>
</CsoundSynthesizer>
