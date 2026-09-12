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
 aPow vosim 1, 128, p4, .25, p6, p5, giPow
 aOther vosim 1, 128, p4, .25, p6, p5, giOther
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
  if !(abs(kPow-kExpected)+abs(kOther-kExpected) < .00001) then
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

instr 99
 if i(gkChecks) != 10 then
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
i 99 .3125 .001
e
</CsScore>
</CsoundSynthesizer>
