<CsTest>
description = "pvstrace restores its bin array and clears old selections"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckArray
 kInput[] fillarray .1,0, .2,512, .3,1024, .4,1536, .5,2048, .6,2560, .7,3072, .8,3584, .9,4096
 kCycle init 0
 kCycle += 1
 ; Change the requested count so a shorter result must clear old indices.
 kKeep = (kCycle % 2 == 0 ? 1 : 3)
 fInput pvsfromarray kInput, 4
 fOutput, kBins[] pvstrace fInput, kKeep
 kLength lenarray kBins
 if kLength != 9 then
  printks "pvstrace bin array length: expected 9, got %g\n", 0, kLength
  exitnowk(-1)
 endif
 kIndex = 0
 while kIndex < 9 do
  kExpected = (kIndex < kKeep ? 9-kKeep+kIndex : 0)
  if kBins[kIndex] != kExpected then
   printks "pvstrace reused array: cycle=%g index=%g expected=%g actual=%g\n", 0, kCycle, kIndex, kExpected, kBins[kIndex]
   exitnowk(-1)
  endif
  kIndex += 1
 od
 ; Retain capacity while changing the length seen by the next frame.
 if kCycle % 2 == 0 then
  trim kBins, 0
 else
  trim kBins, 4
 endif
 if kCycle == 16 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 1 then
  prints "pvstrace array checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "CheckArray" 0 .02
i "CheckResults" .03 .01
e
</CsScore>
</CsoundSynthesizer>
