<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
giWave16 ftgen 1, 0, 16, -7, 1, 16, 1
giWave15 ftgen 2, 0, -15, -7, 1, 15, 1
giFreq ftgen 3, 0, 4, -2, 0, 0, 0, 0
giAmp ftgen 4, 0, 4, -2, 1, .5, .25, 0
gkPrevious0 init 0
gkPrevious1 init 0
gkPrevious2 init 0
gkNotes init 0

instr 1
  ; p4 block size, p5 wavetable, p6 interpolation, p7 initial phase/skip.
  setksmps p4
  iSize = p4
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart-iStart%iSize
  iInitial = (p6 == 0 ? 0 : .0001)
  kPrevious0 init (p7 >= 0 ? iInitial : i(gkPrevious0))
  kPrevious1 init (p7 >= 0 ? iInitial : i(gkPrevious1))
  kPrevious2 init (p7 >= 0 ? iInitial : i(gkPrevious2))
  kCycle init 0
  kAmplitude = (kCycle%4 == 0 ? .8 : (kCycle%4 == 1 ? .2 : (kCycle%4 == 2 ? 0 : -.4)))
  aOutput adsynt2 kAmplitude, 0, p5, giFreq, giAmp, 3, p7, p6
  kTarget0 = kAmplitude
  kTarget1 = kAmplitude*.5
  kTarget2 = kAmplitude*.25
  if p6 != 0 then
    kTarget0 = (kTarget0 > 0 ? kTarget0 : .0001)
    kTarget1 = (kTarget1 > 0 ? kTarget1 : .0001)
    kTarget2 = (kTarget2 > 0 ? kTarget2 : .0001)
  endif
  kBlock = iBlock+kCycle*iSize
  kStart = max(iStart-kBlock, 0)
  kEnd = min(iEnd-kBlock, iSize)
  kN = 0
  while kN < iSize do
    if kN >= kStart && kN < kEnd then
      kFraction = (kN-kStart)/(kEnd-kStart)
      if p6 == 0 then
        kPrevious = kPrevious0+kPrevious1+kPrevious2
        kTarget = kTarget0+kTarget1+kTarget2
        kExpected = kPrevious + (kTarget-kPrevious)*kFraction
      else
        kExpected = kPrevious0*pow(kTarget0/kPrevious0,kFraction) + kPrevious1*pow(kTarget1/kPrevious1,kFraction) + kPrevious2*pow(kTarget2/kPrevious2,kFraction)
      endif
    else
      kExpected = 0
    endif
    kActual vaget kN, aOutput
    if abs(kActual-kExpected) > .00001 then
      printks "adsynt2 mode %g table %g sample %g: %g, expected %g\n", 0, p6, p5, kBlock+kN, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kPrevious0 = kTarget0
  kPrevious1 = kTarget1
  kPrevious2 = kTarget2
  gkPrevious0 = kPrevious0
  gkPrevious1 = kPrevious1
  gkPrevious2 = kPrevious2
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 99
  if i(gkNotes) != 14 then
    prints "adsynt2 cases did not all run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, partial first/last blocks with retained state, and one active sample.
i 1 0 .0625 16 1 0 0
i 1 .0654296875 .0205078125 16 1 0 -1
i 1 .1279296875 .0009765625 16 1 0 0
i 1 .25 .0625 16 1 1 0
i 1 .3154296875 .0205078125 16 1 1 -1
i 1 .3779296875 .0009765625 16 1 1 0
; Non-power-of-two wavetable uses floating-point phases.
i 1 .5 .0625 16 2 0 0
i 1 .5654296875 .0205078125 16 2 0 -1
i 1 .6279296875 .0009765625 16 2 0 0
i 1 .75 .0625 16 2 1 0
i 1 .8154296875 .0205078125 16 2 1 -1
i 1 .8779296875 .0009765625 16 2 1 0
; A one-sample block outputs the previous value, then saves the target.
i 1 1 .0078125 1 1 0 0
i 1 1.03125 .0078125 1 2 1 0
i 99 1.125 0
e
</CsScore>
</CsoundSynthesizer>
