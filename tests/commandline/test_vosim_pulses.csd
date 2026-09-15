<CsTest>
description = "vosim pulse state, table paths, and direction changes"

[expect]
exit = 0
</CsTest>
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
; The same two-level pulse in power-of-two and non-power-of-two tables.
giPow ftgen 0, 0, 16, -2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2
giOther ftgen 0, 0, -12, -2, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2

instr 1
 iStart = round(p2*sr)
 iLength = round(p3*sr)
 aPow vosim 1, 128, p4, .25, p6, p5, giPow, 0, 1
 aOther vosim 1, 128, p4, .25, p6, p5, giOther
 aOtherZero vosim 1, 128, p4, .25, p6, p5, giOther, 0, 0
 aOtherOne vosim 1, 128, p4, .25, p6, p5, giOther, 0, 1
 kBlock init 0
 kIndex = 0
 while kIndex < ksmps do
  kPosition = kBlock*ksmps+kIndex-iStart%ksmps
  kExpected = 0
  if kPosition >= 0 && kPosition < iLength then
   kTime = kPosition%64
   kPulse = 0
   kBegin = 0
   kLength = (p4 == 0 ? 128 : sr/abs(p4))
   kDirection = (p4 < 0 ? -1 : 1)
   ; Only whole pulses that fit in the 64-sample event may start.
   while kPulse < p6 && kBegin+kLength <= 64 do
    if kTime >= kBegin && kTime < kBegin+kLength then
     kValue = (kTime-kBegin < kLength/2 ? 1 : 2)
     if kDirection < 0 then
      kValue = 3-kValue
     endif
     kExpected = (1-.25*kPulse)*kValue
    endif
    kBegin += kLength
    kLength = (p5 == 0 ? 128 : kLength/abs(p5))
    if p5 < 0 then
     kDirection = -kDirection
    endif
    kPulse += 1
   od
  endif
  kPow vaget kIndex, aPow
  kOther vaget kIndex, aOther
  kOtherZero vaget kIndex, aOtherZero
  kOtherOne vaget kIndex, aOtherOne
  if !(abs(kPow-kExpected)+abs(kOther-kExpected) + \
       abs(kOtherZero-kExpected)+abs(kOtherOne-kExpected) < .00001) then
   printks "vosim form=%g factor=%g sample=%g pow=%g other=%g expected=%g\n", 0, p4, p5, kPosition, kPow, kOther, kExpected
   exitnowk(-1)
  endif
  kIndex += 1
 od
 if kBlock == 0 then
  gkChecks += 1
 endif
 kBlock += 1
endin

instr 2
 aDefault vosim 1, 128, 512, .25, 3, 1, giPow
 aZero vosim 1, 128, 512, .25, 3, 1, giPow, 0, 0
 aStopped vosim 1, 128, 512, .25, 3, 0, giPow, 0, 0
 kBlock init 0
 ; Legacy output uses the amplitude saved at the start of each block.
 kAmplitude = kBlock == 0 ? 0 : (kBlock%4 == 0 ? .25 : \
              (kBlock%4 == 1 ? 1 : (kBlock%4 == 2 ? .75 : 0)))
 kIndex = 0
 while kIndex < ksmps do
  kExpected = kAmplitude * (kIndex < 8 ? 1 : 2)
  kDefault vaget kIndex, aDefault
  kZero vaget kIndex, aZero
  kStopped vaget kIndex, aStopped
  if !(abs(kDefault-kExpected)+abs(kZero-kExpected)+abs(kStopped) < .00001) then
   printks "vosim legacy block=%g sample=%g default=%g zero=%g expected=%g\n", 0, kBlock, kIndex, kDefault, kZero, kExpected
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
 if i(gkChecks) != 11 then
  prints "vosim checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 512 1 3
i 1 .03125 .015625 -512 1 3
i 1 .0625 .015625 512 -1 3
i 1 .09375 .015625 -512 -1 3
i 1 .125 .015625 512 0 3
i 1 .15625 .015625 512 2 3
i 1 .1875 .015625 512 .5 3
i 1 .21875 .015625 0 1 3
i 1 .25 .015625 512 1 0
i 1 .2818603515625 .0123291015625 512 -1 3
i 2 .3125 .03125
i 99 .359375 .001
e
</CsScore>
</CsoundSynthesizer>
