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
giOne ftgen 0, 0, 8, -2, 0, 1, 2, 3, 4, 5, 6, 7
giTwo ftgen 0, 0, 8, -2, 10, 12, 14, 16, 18, 20, 22, 24

instr 1
 iTables[] fillarray giOne, giTwo
 iWeight limit p4, 0, 1
 iStart = round(p2*sr)
 iLength = round(p3*sr)
 kPlain tabmorph 2.5, p4, .25, 1.5, giOne, giTwo
 kPlainArray tabmorph 2.5, p4, .25, 1.5, iTables
 kInterp tabmorphi 2.5, p4, .25, 1.5, giOne, giTwo
 kInterpArray tabmorphi 2.5, p4, .25, 1.5, iTables
 kError = abs(kPlain-(5+3*iWeight))+abs(kPlainArray-(5+3*iWeight))
 kError += abs(kInterp-(5.625+3.125*iWeight))+abs(kInterpArray-(5.625+3.125*iWeight))
 if !(kError < .00001) then
  printks "tabmorph control weight=%g error=%g\n", 0, p4, kError
  exitnowk(-1)
 endif
 aIndex phasor sr/16
 aWeight = 2*aIndex-.5
 aOriginal = aWeight
 aFirst = .25
 aSecond = 1.5
 aAudio tabmorpha aIndex, aWeight, aFirst, aSecond, giOne, giTwo
 aAudioArray tabmorpha aIndex, aWeight, aFirst, aSecond, iTables
 aControl tabmorphak aIndex, p4, .25, 1.5, giOne, giTwo
 aControlArray tabmorphak aIndex, p4, .25, 1.5, iTables
 aAlias = aOriginal
 aAlias tabmorpha aIndex, aAlias, aFirst, aSecond, giOne, giTwo
 aBase tablei aIndex, giOne, 1
 aOther tablei aIndex, giTwo, 1
 kBlock init 0
 kIndex = 0
 while kIndex < ksmps do
  kPosition = kBlock*ksmps+kIndex-iStart%ksmps
  kExpected = 0
  kExpectedControl = 0
  kOriginal vaget kIndex, aOriginal
  kWeight vaget kIndex, aWeight
  if kPosition >= 0 && kPosition < iLength then
   kBase vaget kIndex, aBase
   kOther vaget kIndex, aOther
   kStart = .75*kBase+.25*kOther
   kEnd = .5*(kBase+kOther)
   kMix limit kOriginal, 0, 1
   kExpected = kStart+(kEnd-kStart)*kMix
   kExpectedControl = kStart+(kEnd-kStart)*iWeight
  endif
  kAudio vaget kIndex, aAudio
  kAudioArray vaget kIndex, aAudioArray
  kControl vaget kIndex, aControl
  kControlArray vaget kIndex, aControlArray
  kAlias vaget kIndex, aAlias
  kError = abs(kAudio-kExpected)+abs(kAudioArray-kExpected)+abs(kAlias-kExpected)
  kError += abs(kControl-kExpectedControl)+abs(kControlArray-kExpectedControl)
  kError += abs(kWeight-kOriginal)
  if !(kError < .00001) then
   printks "tabmorph audio weight=%g sample=%g error=%g\n", 0, kOriginal, kPosition, kError
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kBlock == 0 then
  gkChecks += 1
 endif
 kBlock += 1
endin

instr 99
 if i(gkChecks) != 6 then
  prints "tabmorph checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125 -.5
i 1 .015625 .0078125 0
i 1 .03125 .0078125 .5
i 1 .046875 .0078125 1
i 1 .0625 .0078125 1.5
i 1 .0806884765625 .0042724609375 1
i 99 .1 .001
e
</CsScore>
</CsoundSynthesizer>
