<CsTest>
description = "DBAP accepts padded 3D tables and optional or extra speaker weights"
[expect]
exit = 0
</CsTest>
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
; Two triples in a power-of-two table; the last two cells are padding.
giPositions ftgen 0, 0, 8, -2, -1, 0, 0, 1, 0, 0, 100, 100
giWeights ftgen 0, 0, 4, -2, 1, 2, 100, 100

instr 1
  iWeights[] fillarray 1, 2, 100
  kSource[] fillarray 0, 0, 0
  kArray[] init 2
  kTable[] init 2
  kDefault[] init 2
  aArray[] init 2
  aTable[] init 2
  aDefault[] init 2
  aInput = .25
  kArray dbapgains 0, kSource, giPositions, 0, 6, 3, iWeights
  kTable dbapgains 0, kSource, giPositions, 0, 6, 3, giWeights
  kDefault dbapgains 0, kSource, giPositions, 0, 6, 3
  aArray dbap aInput, 0, kSource, giPositions, 0, 6, 3, iWeights
  aTable dbap aInput, 0, kSource, giPositions, 0, 6, 3, giWeights
  aDefault dbap aInput, 0, kSource, giPositions, 0, 6, 3
  kCycle init 0
  kChannel = 0
  while kChannel < 2 do
    kExpected = (kChannel + 1)/sqrt(5)
    kUnity = 1/sqrt(2)
    if !(abs(kArray[kChannel]-kExpected) < .00001 && \
         abs(kTable[kChannel]-kExpected) < .00001 && \
         abs(kDefault[kChannel]-kUnity) < .00001) then
      printks "DBAP padded table gains differ from expected weights\n", 0
      exitnowk(-1)
    endif
    aOne = aArray[kChannel]
    aTwo = aTable[kChannel]
    aThree = aDefault[kChannel]
    kSample = 0
    while kSample < ksmps do
      kScale = .25
      if kCycle == 0 then
        kScale *= kSample/ksmps
      endif
      kOne vaget kSample, aOne
      kTwo vaget kSample, aTwo
      kThree vaget kSample, aThree
      if !(abs(kOne-kScale*kExpected) < .00001 && \
           abs(kTwo-kScale*kExpected) < .00001 && \
           abs(kThree-kScale*kUnity) < .00001) then
        printks "DBAP padded table audio differs from expected weights\n", 0
        exitnowk(-1)
      endif
      kSample += 1
    od
    kChannel += 1
  od
  kCycle += 1
  if kCycle == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 1 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
