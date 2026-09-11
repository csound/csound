<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 aX, aY, aZ lorenz 10, 28, 8/3, .001, 1, 1, 1, p4
 aGate = 1
 kX init 1
 kY init 1
 kZ init 1
 kCount init 0
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kActualX vaget kN, aX
  kActualY vaget kN, aY
  kActualZ vaget kN, aZ
  kExpectedX = 0
  kExpectedY = 0
  kExpectedZ = 0
  if kActive != 0 then
   kStep = 0
   while kStep < max(1, int(p4)) do
    ; Euler integration uses all three coordinates from the same step.
    kNextX = kX+.001*10*(kY-kX)
    kNextY = kY+.001*(-kX*kZ+28*kX-kY)
    kZ = kZ+.001*(kX*kY-(8/3)*kZ)
    kX = kNextX
    kY = kNextY
    kStep += 1
   od
   kExpectedX = kX
   kExpectedY = kY
   kExpectedZ = kZ
  endif
  kError = abs(kActualX-kExpectedX)+abs(kActualY-kExpectedY)+abs(kActualZ-kExpectedZ)
  if !(kError < .0001) then
   printks "lorenz skip=%g block=%g sample=%g error=%g\n", 0, p4, kCount, kN, kError
   exitnowk(-1)
  endif
  kN += 1
 od
 kCount += 1
 if kCount == 8 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 8 then
  prints "lorenz checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02 1
i 1 0 .02 4
i 1 0 .02 4.5
i 1 0 .02 0
i 1 0 .02 -1
i 1 .030625 .01975 1
i 1 .030625 .01975 4
i 1 .030625 .01975 8
i 99 .06 .002
e
</CsScore>
</CsoundSynthesizer>
