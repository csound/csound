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
giPositions ftgen 0, 0, 4, -2, -1, 0, 1, 0

instr 1
 iWeightTable ftgen 0, 0, 2, -2, p4, p5
 iWeights[] fillarray p4, p5
 iPositions[][] init 2, 2
 iPositions[0][0] = -1
 iPositions[1][0] = 1
 kSource[] fillarray p6, 0
 iNorm = sqrt(p4*p4+p5*p5)
 if iNorm == 0 then
  iNorm = 1
 endif
 kAA[] init 2
 kAT[] init 2
 kTA[] init 2
 kTT[] init 2
 aAA[] init 2
 aAT[] init 2
 aTA[] init 2
 aTT[] init 2
 aInput = .25
 ; Cover all array/table forms of the shared gain calculation.
 kAA dbapgains 0, kSource, iPositions, 0, 6.02059991327962, iWeights
 kAT dbapgains 0, kSource, iPositions, 0, 6.02059991327962, iWeightTable
 kTA dbapgains 0, kSource, giPositions, 0, 6.02059991327962, 2, iWeights
 kTT dbapgains 0, kSource, giPositions, 0, 6.02059991327962, 2, iWeightTable
 aAA dbap aInput, 0, kSource, iPositions, 0, 6.02059991327962, iWeights
 aAT dbap aInput, 0, kSource, iPositions, 0, 6.02059991327962, iWeightTable
 aTA dbap aInput, 0, kSource, giPositions, 0, 6.02059991327962, 2, iWeights
 aTT dbap aInput, 0, kSource, giPositions, 0, 6.02059991327962, 2, iWeightTable
 kCycle init 0
 kChannel = 0
 while kChannel < 2 do
  ; Equal speaker distances leave only the normalized weights.
  kExpected = iWeights[kChannel]/iNorm
  kError = abs(kAA[kChannel]-kExpected)+abs(kAT[kChannel]-kExpected)
  kError += abs(kTA[kChannel]-kExpected)+abs(kTT[kChannel]-kExpected)
  if !(kError < .00001) then
   printks "dbap weights=(%g,%g) channel=%g gains error=%g\n", 0, p4, p5, kChannel, kError
   exitnowk(-1)
  endif
  aOne = aAA[kChannel]
  aTwo = aAT[kChannel]
  aThree = aTA[kChannel]
  aFour = aTT[kChannel]
  kIndex = 0
  while kIndex < ksmps do
   kAudioExpected = .25*kExpected
   if kCycle == 0 then
    kAudioExpected *= kIndex/ksmps
   endif
   kOne vaget kIndex, aOne
   kTwo vaget kIndex, aTwo
   kThree vaget kIndex, aThree
   kFour vaget kIndex, aFour
   kError = abs(kOne-kAudioExpected)+abs(kTwo-kAudioExpected)
   kError += abs(kThree-kAudioExpected)+abs(kFour-kAudioExpected)
   if !(kError < .00001) then
    printks "dbap weights=(%g,%g) channel=%g audio error=%g\n", 0, p4, p5, kChannel, kError
    exitnowk(-1)
   endif
   kIndex += 1
  od
  kChannel += 1
 od
 kCycle += 1
 if kCycle == 4 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 6 then
  prints "dbap weight checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125 1 1 0
i 1 .015625 .0078125 0 1 0
i 1 .03125 .0078125 1 0 0
i 1 .046875 .0078125 2 1 0
i 1 .0625 .0078125 0 0 0
i 1 .078125 .0078125 0 0 .5
i 99 .1 .001
e
</CsScore>
</CsoundSynthesizer>
