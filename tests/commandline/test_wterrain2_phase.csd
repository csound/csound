<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
giRamp ftgen 0, 0, 16384, -7, 0, 16384, 1
gkChecks init 0

instr 1
 kCycle init 0
 kCycle += 1
 kFrequency = kCycle < 200 ? p5 : (kCycle < 220 ? 0 : -p5)
 aActual wterrain2 1, kFrequency, .5, .5, .2, .2, .4, giRamp, giRamp, p4, .3
 ; A normalized phasor supplies an independent, bounded phase reference.
 ; With ramp tables the terrain is simply the product of its coordinates,
 ; apart from the rounding to table indices.
 aPhase phasor kFrequency
 aT = 2*$M_PI*aPhase
 aSin = sin(aT)
 aCos = cos(aT)
 if p4 == 0 then
  aX = sin(aT+.3*aSin)
  aY = cos(aT+.3*aSin)
 elseif p4 == 1 then
  aX = cos(aT+.3*aSin)
  aY = sin(aT+.3*aSin)*aX
 elseif p4 == 2 then
  aX = aSin*(aCos+.3)
  aY = aCos*(aCos+.3)
 elseif p4 == 3 then
  aX = aCos*cos(2*aT)
  aY = aSin*(.3+cos(2*aT))
 elseif p4 == 4 then
  aX = aCos*(1+.3*sin(2*aT))
  aY = aSin*(1+.3*sin(2*aT))
 elseif p4 == 5 then
  aX = aCos*(.3*sin(2*aT)+aSin)
  aY = aSin*(.3*sin(2*aT)+aSin)
 elseif p4 == 6 then
  aX = aCos*aCos*(aSin*aSin-.3)
  aY = aSin*aCos*(aSin*aSin-.3)
 else
  aX = aCos*(1+.3*aSin*aSin)
  aY = aSin*(1-.3-.3*aCos*aCos)
 endif
 aRotX = .5+.2*(aX*cos(.4)-aY*sin(.4))
 aRotY = .5+.2*(aX*sin(.4)+aY*cos(.4))
 aGate = 1
 aExpected = aRotX*aRotY*aGate
 aError = abs(aActual-aExpected)
 kN = 0
 while kN < ksmps do
  kError vaget kN, aError
  if !(kError < .0002) then
   printks "wterrain2 phase mismatch: curve=%g frequency=%g cycle=%g sample=%g error=%g\n", 0, p4, kFrequency, kCycle, kN, kError
   exitnowk(-1)
  endif
  kN += 1
 od
 if kCycle == 490 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 16 then
  prints "wterrain2 checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 1.001375 0 1000
i 1 0 1.001375 1 1000
i 1 0 1.001375 2 1000
i 1 0 1.001375 3 1000
i 1 0 1.001375 4 1000
i 1 0 1.001375 5 1000
i 1 0 1.001375 6 1000
i 1 0 1.001375 7 1000
i 1 .000625 1.00075 0 -1000
i 1 .000625 1.00075 1 -1000
i 1 .000625 1.00075 2 -1000
i 1 .000625 1.00075 3 -1000
i 1 .000625 1.00075 4 -1000
i 1 .000625 1.00075 5 -1000
i 1 .000625 1.00075 6 -1000
i 1 .000625 1.00075 7 -1000
i 99 1.02 .01
e
</CsScore>
</CsoundSynthesizer>
