<CsTest>
description = "vphaseseg interpolation, phase wrapping, empty vectors, and reinitialization"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 8
nchnls = 1
0dbfs = 1
giA ftgen 0, 0, 4, -2, 0, 100, -3, 99
giB ftgen 0, 0, 4, -2, 10, 110, 7, 99
giC ftgen 0, 0, 4, -2, 30, 130, 27, 99
gkChecks init 0

instr 1
 iOut ftgen 0, 0, 4, -2, 77, 77, 77, 77
 vphaseseg p4, iOut, p6, giA, 1, giB, 3, giC
 kFirst table 0, iOut
 kSecond table 1, iOut
 kThird table 2, iOut
 kTail table 3, iOut
 if p6 == 0 then
  kError = abs(kFirst-77) + abs(kSecond-77) + abs(kThird-77)
 else
  kError = abs(kFirst-p5) + abs(kSecond-100-p5) + abs(kThird+3-p5)
 endif
 if !(kError < .0001) || kTail != 77 then
  printks "vphaseseg phase %g: wrong vector (%g, %g, %g, %g)\n", 0, p4, kFirst, kSecond, kThird, kTail
  exitnowk -1
 endif
 kOnce init 1
 if kOnce == 1 then
  gkChecks += 1
  kOnce = 0
 endif
endin

instr 2
 iOut ftgen 0, 0, 4, -2, 0
 kCycle timeinstk
 kDistance init 1
 if kCycle == 3 then
  kDistance = 3
  reinit AGAIN
 endif
AGAIN:
 iDistance = i(kDistance)
 vphaseseg .25, iOut, 3, giA, iDistance, giB, 1, giC
 rireturn
 kValue table 0, iOut
 kExpected = (kCycle < 3 ? 5 : 10/3)
 if abs(kValue-kExpected) > .00001 then
  printks "vphaseseg retained the old segment distances\n", 0
  exitnowk -1
 endif
 if kCycle == 4 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 12 then
  prints "vphaseseg checks did not complete\n"
  exitnow -1
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 -.5 0 3
i 1 .03125 .015625 0 0 3
i 1 .0625 .015625 .125 5 3
i 1 .09375 .015625 .25 10 3
i 1 .125 .015625 .625 20 3
i 1 .15625 .015625 .999 29.9733333333333 3
i 1 .1875 .015625 1 0 3
i 1 .21875 .015625 1.25 10 3
i 1 .25 .015625 1e20 0 3
i 1 .28125 .015625 0 0 0
i 1 .3125 .015625 .625 0 0
i 2 .34375 .046875
i 99 .5 .0078125
e
</CsScore>
</CsoundSynthesizer>
