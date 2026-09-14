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

instr 1
 iOrder = (p4 == 0 ? 4 : p4)
 iStart = round(p2*sr)
 iLength = round(p3*sr)
 iRadius = exp(-2*$M_PI*.1)
 iAngle = 2*$M_PI*p5/sr
 kBlock init 0
 kSample init 0
 kEnvelope init 2*(1-iRadius)^iOrder
 aImpulse mpulse 1, 0
 aOut gtf aImpulse, p5, .1, p4, p6
 aInPlace = aImpulse
 aInPlace gtf aInPlace, p5, .1, p4, p6
 kIndex = 0
 while kIndex < ksmps do
  kPosition = kBlock*ksmps+kIndex-iStart%ksmps
  kExpected = 0
  if kPosition >= 0 && kPosition < iLength then
   ; A cascade of identical complex poles has this closed-form response.
   kExpected = kEnvelope*cos(kSample*iAngle+p6)
   kEnvelope *= iRadius*(kSample+iOrder)/(kSample+1)
   kSample += 1
  endif
  kActual vaget kIndex, aOut
  kInPlace vaget kIndex, aInPlace
  kError = abs(kActual-kExpected)+abs(kInPlace-kExpected)
  if !(kError < .00001) then
   printks "gtf order=%g phase=%g sample=%g actual=%g expected=%g error=%g\n", 0, p4, p6, kPosition, kActual, kExpected, kError
   exitnowk(-1)
  endif
  kIndex += 1
 od
 kBlock += 1
 if kSample == iLength then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 8 then
  prints "gtf checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Orders, output phases, DC and nonzero center frequencies, partial blocks.
i 1 0 .0078125 1 1024 0
i 1 .015625 .0078125 4 1024 0
i 1 .03125 .0078125 10 1024 0
i 1 .046875 .0078125 0 1024 .7853981633974483
i 1 .0625 .0078125 4 0 0
i 1 .078125 .0078125 4 1024 1.5707963267948966
i 1 .0943603515625 .0072021484375 4 1024 .7853981633974483
i 1 .1102294921875 .0003662109375 1 1024 0
i 99 .125 .001
e
</CsScore>
</CsoundSynthesizer>
