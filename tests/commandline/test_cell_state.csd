<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
gkChecks init 0
giInitial ftgen 0, 0, 64, -2, 1
giRule90 ftgen 0, 0, 8, -2, 0, 1, 0, 1, 1, 0, 1, 0

; Rule 90 moves a single live cell to its two neighbors on a circular row.
; Check the existing trigger order, held output, and reset with either trigger.
instr 1
 iOutput ftgen 0, 0, 64, -2, 0
 kCycle init 0
 kCycle += 1
 kTrigger = (kCycle == 2 || kCycle == 4 || kCycle == 5 || kCycle >= 9 && kCycle <= 11 ? 1 : 0)
 kReset = (kCycle == 7 || kCycle == 10 ? 1 : 0)
 cell kTrigger, kReset, iOutput, giInitial, giRule90, p4
 kGeneration = (kCycle == 4 || kCycle >= 11 ? 1 : (kCycle == 5 || kCycle == 6 ? 2 : 0))
 kIndex = 0
 while kIndex < p4 do
  kValue table kIndex, iOutput
  if kGeneration == 0 then
   kExpected = (kIndex == 0 ? 1 : 0)
  elseif p4 == 1 then
   kExpected = 0
  else
   kExpected = (kIndex == kGeneration || kIndex == p4-kGeneration ? 1 : 0)
  endif
  if kValue != kExpected then
   printks "cell generation mismatch: cycle=%g element=%g got=%g expected=%g\n", 0, kCycle, kIndex, kValue, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kCycle == 12 then
  gkChecks += 1
 endif
endin

; Sharing the output and rule tables must not overwrite rules before use.
instr 2
 iInitial ftgen 0, 0, 8, -2, 1
 iShared ftgen 0, 0, 8, -2, 0, 0, 1, 1, 1, 1, 0, 0
 kCycle init 0
 kCycle += 1
 cell 1, 0, iShared, iInitial, iShared, 8
 if kCycle == 2 then
  kIndex = 0
  while kIndex < 8 do
   kValue table kIndex, iShared
   kExpected = (kIndex < 2 ? 1 : 0)
   if kValue != kExpected then
    printks "cell changed a rule while computing the generation\n", 0
    exitnowk(-1)
   endif
   kIndex += 1
  od
  gkChecks += 1
 endif
endin

; Reinitializing can change the row length while retaining the same tables.
instr 3
 iOutput ftgen 0, 0, 64, -2, 0
 kCycle init 0
 kCycle += 1
 if kCycle == 7 then
  reinit SETUP
 endif
 kStep = (kCycle < 7 ? kCycle : kCycle-6)
 kTrigger = (kStep == 2 || kStep == 4 ? 1 : 0)
SETUP:
 iCount = (i(kCycle) < 7 ? p4 : p5)
 cell kTrigger, 0, iOutput, giInitial, giRule90, iCount
 rireturn
 kCount = iCount
 kIndex = 0
 while kIndex < kCount do
  kValue table kIndex, iOutput
  kExpected = (kStep < 4 ? (kIndex == 0 ? 1 : 0) : (kIndex == 1 || kIndex == kCount-1 ? 1 : 0))
  if kValue != kExpected then
   printks "cell reinit mismatch: cycle=%g element=%g got=%g expected=%g\n", 0, kCycle, kIndex, kValue, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kCycle == 12 then
  gkChecks += 1
 endif
endin

; Fractional states still use the truncated weighted neighborhood index.
instr 4
 iInitial ftgen 0, 0, 4, -2, .25, .5, .75, 0
 iRules ftgen 0, 0, 8, -2, 0, .125, .25, .375, .5, .625, .75, .875
 iOutput ftgen 0, 0, 4, -2, 0
 iExpected ftgen 0, 0, 4, -2, .125, .25, .375, .375
 kCycle init 0
 kCycle += 1
 cell 1, 0, iOutput, iInitial, iRules, 4
 if kCycle == 2 then
  kIndex = 0
  while kIndex < 4 do
   kValue table kIndex, iOutput
   kExpected table kIndex, iExpected
   if kValue != kExpected then
    printks "cell fractional rule lookup mismatch\n", 0
    exitnowk(-1)
   endif
   kIndex += 1
  od
  gkChecks += 1
 endif
endin

; A weighted index between -1 and 0 still truncates to rule zero.
instr 5
 iInitial ftgen 0, 0, 8, -2, -.125
 iRules ftgen 0, 0, 8, -2, .5
 iOutput ftgen 0, 0, 8, -2, 0
 kCycle init 0
 kCycle += 1
 cell 1, 0, iOutput, iInitial, iRules, 1
 if kCycle == 2 then
  kValue table 0, iOutput
  if kValue != .5 then
   printks "cell negative fractional index did not select rule zero\n", 0
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 8 then
  prints "cell checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .09375 1
i 1 0 .09375 8
i 1 0 .09375 64
i 2 0 .015625
i 3 0 .09375 8 64
i 3 0 .09375 64 8
i 4 0 .015625
i 5 0 .015625
i 99 .1 .01
e
</CsScore>
</CsoundSynthesizer>
