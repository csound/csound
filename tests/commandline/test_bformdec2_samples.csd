<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps=16
nchnls=2
0dbfs=1
gaStereo[] init 2
gaStereo[0] init 9
gaStereo[1] init 9
gkNotes init 0
gkPrefixChecks init 0

instr 1
  aInput[] init 4
  aOne upsamp 1
  aInput[0] = aOne
  gaStereo bformdec2 1, aInput
endin

; Observe the raw output from a full block so array reads retain its prefix.
instr 2
  kCycle init 0
  aLeft = gaStereo[0]
  aRight = gaStereo[1]
  kN = 0
  while kN < ksmps do
    kSample = kCycle*ksmps+kN
    kExpected = (kSample >= 3 && kSample < 24 ? sqrt(.5) : 0)
    kLeft vaget kN, aLeft
    kRight vaget kN, aRight
    if abs(kLeft-kExpected) > .000001 || abs(kRight-kExpected) > .000001 then
      printks "bformdec2 stereo sample %g: %g %g, expected %g\n", 0, kSample, kLeft, kRight, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCycle += 1
  gkPrefixChecks += 1
endin

instr 10
  ; p4 decoder, p5 distance, p6 input channels, p7 setup, p8 outputs, p9 crossover.
  aInput[] init p6
  aOriginal[] init p6
  aCopy[] init p6
  aFirst[] init p8
  aSecond[] init p8
  aSeparate[] init p8
  aReuse[] init p6
  aBase oscili .25, 256
  aBase += .5
  kCh = 0
  while kCh < p6 do
    aInput[kCh] = aBase * ((kCh+1)*.01)
    kCh += 1
  od
  aOriginal = aInput
  aCopy = aInput
  aReuse = aInput
  aFirst bformdec2 p7, aInput, p4, p5, p9
  aSecond bformdec2 p7, aInput, p4, p5, p9
  aSeparate bformdec2 p7, aCopy, p4, p5, p9
  if p7 == 2 then
    aReuse bformdec2 p7, aReuse, p4, p5, p9
  endif

  ; A first-order NFC reference, followed by independent crossover filters.
  iDistance = (p5 == 0 ? 1 : p5)
  if p5 == -1 then
    aDirection = aBase
  else
    iOmega = 343.2/(iDistance*sr)
    iGain = 1/(1+iOmega/2)
    iFeedback = 1-iOmega/(1+iOmega/2)
    aDirection biquad aBase, iGain, -iGain, 0, 1, -iFeedback, 0
  endif
  iG = tan($M_PI*p9/sr)
  iDen = (1+iG)*(1+iG)
  iA1 = 2*(iG*iG-1)/iDen
  iA2 = (1-iG)*(1-iG)/iDen
  iLP = iG*iG/iDen
  iHP = 1/iDen
  aLPW biquad aBase, iLP, 2*iLP, iLP, 1, iA1, iA2
  aHPW biquad aBase, iHP, -2*iHP, iHP, 1, iA1, iA2
  aLPD biquad aDirection, iLP, 2*iLP, iLP, 1, iA1, iA2
  aHPD biquad aDirection, iHP, -2*iHP, iHP, 1, iA1, iA2

  kCh = 0
  while kCh < p6 do
    aDifference = aInput[kCh]-aOriginal[kCh]
    kN = 0
    while kN < ksmps do
      kDifference vaget kN, aDifference
      if kDifference != 0 then
        printks "bformdec2 changed input channel %g\n", 0, kCh
        exitnowk -1
      endif
      kN += 1
    od
    kCh += 1
  od
  kCh = 0
  while kCh < p8 do
    aFirstChannel = aFirst[kCh]
    aSecondChannel = aSecond[kCh]
    aSeparateChannel = aSeparate[kCh]
    if p7 == 2 then
      aReuseChannel = aReuse[kCh]
    else
      aReuseChannel = aFirstChannel
    endif
    if p6 == 4 then
      kPair = (p7 == 5 ? int(kCh/2) : kCh)
      kX = (kPair == 0 || kPair == 3 ? 1 : -1)
      kY = (kPair < 2 ? 1 : -1)
      if p7 == 2 then
        kW = .3535533906*.01
        kD = .3535533906*(kX*.02+kY*.03)
        kHighW = sqrt(2)
        kHighD = 1
      else
        kZ = (kCh%2 == 0 ? -1 : 1)
        kW = .1767766953*.01
        kD = .2165063509*(kX*.02+kY*.03+kZ*.04)
        kHighW = 2
        kHighD = sqrt(2)
      endif
      if p4 == 0 then
        aExpected = kW*aLPW+kD*aLPD-kW*kHighW*aHPW-kD*kHighD*aHPD
      elseif p4 == 1 then
        aExpected = kW*aBase+kD*aDirection
      else
        aExpected = kW*kHighW*aBase+kD*kHighD*aDirection
      endif
    else
      aExpected = aSeparateChannel
    endif
    kN = 0
    while kN < ksmps do
      kFirst vaget kN, aFirstChannel
      kSecond vaget kN, aSecondChannel
      kSeparate vaget kN, aSeparateChannel
      kReuse vaget kN, aReuseChannel
      kExpected vaget kN, aExpected
      if abs(kFirst-kExpected) > .000001 || abs(kSecond-kFirst) > .000001 || abs(kSeparate-kFirst) > .000001 || abs(kReuse-kFirst) > .000001 then
        printks "bformdec2 setup %g decoder %g distance %g channel %g: %g %g %g %g, expected %g\n", 0, p7, p4, p5, kCh, kFirst, kSecond, kSeparate, kReuse, kExpected
        exitnowk -1
      endif
      kN += 1
    od
    kCh += 1
  od
  kCycle init 0
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 99
  if i(gkPrefixChecks) != 2 || i(gkNotes) != 12 then
    prints "bformdec2 cases did not all run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 .0003662109375 .0025634765625
i 2 0 .00390625
i 10 .03125 .03125 0 -1 4 2 4 400
i 10 .03125 .03125 1 -1 4 2 4 400
i 10 .03125 .03125 2 -1 4 2 4 400
i 10 .0941162109375 .0205078125 0 .5 4 2 4 400
i 10 .0941162109375 .0205078125 1 .5 4 2 4 1000
i 10 .0941162109375 .0205078125 2 .5 4 2 4 1000
i 10 .15625 .03125 0 1 4 5 8 400
i 10 .15625 .03125 1 1 4 5 8 1000
i 10 .15625 .03125 2 1 4 5 8 1000
; Higher orders also preserve shared input with NFC enabled.
i 10 .2191162109375 .0205078125 0 1 9 4 8 400
i 10 .2191162109375 .0205078125 0 1 16 4 8 400
i 10 .28125 .03125 0 0 4 2 4 400
i 99 .375 0
e
</CsScore>
</CsoundSynthesizer>
