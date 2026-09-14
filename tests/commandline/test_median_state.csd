<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; Compare both rates against a sorted history, including reinitialization
; away from the ring boundary and changes in the active window size.
instr 1
 setksmps 1
 kCycle init 0
 kNext init 0
 kHistory[] init 5
 kSorted[] init 5
 kCycle += 1
 kInput = (kCycle * 7) % 13 - 6
 kWindow = (kCycle - 1) % 5 + 1
 if kCycle == 8 || kCycle == 13 || kCycle == 21 then
  reinit FILTER
  if p4 == 0 then
   kNext = 0
   kIndex = 0
   while kIndex < 5 do
    kHistory[kIndex] = 0
    kIndex += 1
   od
  endif
 endif
FILTER:
 aInput = kInput
 aResult median aInput, kWindow, 5, p4
 kResult mediank kInput, kWindow, 5, p4
 rireturn
 kAudio downsamp aResult
 kHistory[kNext] = kInput
 kNext = (kNext + 1) % 5
 kIndex = 0
 while kIndex < kWindow do
  kSorted[kIndex] = kHistory[(kNext + 4 - kIndex) % 5]
  kIndex += 1
 od
 kIndex = 1
 while kIndex < kWindow do
  kOther = kIndex
  while kOther > 0 do
   if kSorted[kOther] < kSorted[kOther - 1] then
    kTemp = kSorted[kOther]
    kSorted[kOther] = kSorted[kOther - 1]
    kSorted[kOther - 1] = kTemp
   endif
   kOther -= 1
  od
  kIndex += 1
 od
 kExpected = kSorted[int((kWindow - 1) / 2)]
 if kAudio != kExpected || kResult != kExpected then
  printks "median skip=%g cycle=%g window=%g: audio=%g control=%g expected=%g\n", 0, p4, kCycle, kWindow, kAudio, kResult, kExpected
  exitnowk(-1)
 endif
 if kCycle == 32 then
  gkChecks += 1
 endif
endin

; A five-sample median of a step reaches one on its third active sample.
; Check inactive samples as well, with separate and reused audio variables.
instr 2
 aInput = 1
 if p4 == 0 then
  aResult median aInput, 5, 5
 else
  aInput median aInput, 5, 5
  aResult = aInput
 endif
 kIndex = 0
 while kIndex < 16 do
  kSample vaget kIndex, aResult
  kExpected = (kIndex >= 5 && kIndex < 11 ? 1 : 0)
  if kSample != kExpected then
   printks "median partial block sample=%g: got=%g expected=%g\n", 0, kIndex, kSample, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 gkChecks += 1
endin
instr 99
 if i(gkChecks) != 4 then
  prints "median checks did not complete: %g\n", i(gkChecks)
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .00390625 0
i 1 .0078125 .00390625 1
i 2 .0159912109375 .0009765625 0
i 2 .0198974609375 .0009765625 1
i 99 .03125 .001953125
e
</CsScore>
</CsoundSynthesizer>
