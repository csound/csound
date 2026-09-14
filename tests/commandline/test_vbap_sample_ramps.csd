<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
zakinit 8, 1
vbaplsinit 2, 2, 0, 90
vbaplsinit 2.03, 2, 90, 0
gkChecks init 0

instr 1
 kBlock init 0
 kBlock += 1
 aInput = .5
 aGate = 1
 aReuse = aInput
 aArray[] init 2
 aArrayReuse[] init 2
 aArrayReuse[0] = aInput
 aLastReuse = aInput
 kAzimuth init 0
 kAzimuth = (kBlock < 3 ? 90 : 0)
 if p4 == 0 then
  aFirst, aSecond vbap aInput, kAzimuth, 0, 0, p5
  aArray vbap aInput, kAzimuth, 0, 0, p5
  aReuse, aReuseSecond vbap aReuse, kAzimuth, 0, 0, p5
  aLastFirst, aLastReuse vbap aLastReuse, kAzimuth, 0, 0, p5
  aArrayReuse vbap aArrayReuse[0], kAzimuth, 0, 0, p5
  vbapz 2, 2, aInput, kAzimuth, 0, 0, p5
  kOldAngle = (kBlock == 1 || kBlock > 3 ? 0 : 90)
  kNewAngle = kAzimuth
 else
  ; The path advances once at init and once per control cycle.
  aFirst, aSecond vbapmove aInput, 16/kr, 0, 2, 0, 90
  aArray vbapmove aInput, 16/kr, 0, 2, 0, 90
  aReuse, aReuseSecond vbapmove aReuse, 16/kr, 0, 2, 0, 90
  aLastFirst, aLastReuse vbapmove aLastReuse, 16/kr, 0, 2, 0, 90
  aArrayReuse vbapmove aArrayReuse[0], 16/kr, 0, 2, 0, 90
  vbapzmove 2, 2, aInput, 16/kr, 0, 2, 0, 90
  kOldAngle = 90*kBlock/16
  kNewAngle = 90*(kBlock+1)/16
 endif
 aZakFirst zar 2
 aZakSecond zar 3
 kOldFirst = cos(kOldAngle*$M_PI/180)
 kNewFirst = cos(kNewAngle*$M_PI/180)
 kOldSecond = sin(kOldAngle*$M_PI/180)
 kNewSecond = sin(kNewAngle*$M_PI/180)
 if p5 != 0 then
  kSwap = kOldFirst
  kOldFirst = kOldSecond
  kOldSecond = kSwap
  kSwap = kNewFirst
  kNewFirst = kNewSecond
  kNewSecond = kSwap
 endif
 kCount = 0
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kCount += kActive
  kN += 1
 od
 kPosition = 0
 kN = 0
 while kN < ksmps do
  kActive vaget kN, aGate
  kExpectedFirst = 0
  kExpectedSecond = 0
  if kActive != 0 then
   kPosition += 1
   kFraction = kPosition/kCount
   kExpectedFirst = .5*(kOldFirst+kFraction*(kNewFirst-kOldFirst))
   kExpectedSecond = .5*(kOldSecond+kFraction*(kNewSecond-kOldSecond))
  endif
  kFirst vaget kN, aFirst
  kSecond vaget kN, aSecond
  kArrayFirst vaget kN, aArray[0]
  kArraySecond vaget kN, aArray[1]
  kZakFirst vaget kN, aZakFirst
  kZakSecond vaget kN, aZakSecond
  kReuseFirst vaget kN, aReuse
  kReuseSecond vaget kN, aReuseSecond
  kLastFirst vaget kN, aLastFirst
  kLastSecond vaget kN, aLastReuse
  kArrayReuseFirst vaget kN, aArrayReuse[0]
  kArrayReuseSecond vaget kN, aArrayReuse[1]
  kError = abs(kFirst-kExpectedFirst)+abs(kSecond-kExpectedSecond)
  kError += abs(kArrayFirst-kExpectedFirst)+abs(kArraySecond-kExpectedSecond)
  kError += abs(kZakFirst-kExpectedFirst)+abs(kZakSecond-kExpectedSecond)
  kError += abs(kReuseFirst-kExpectedFirst)+abs(kReuseSecond-kExpectedSecond)
  kError += abs(kLastFirst-kExpectedFirst)+abs(kLastSecond-kExpectedSecond)
  kError += abs(kArrayReuseFirst-kExpectedFirst)+abs(kArrayReuseSecond-kExpectedSecond)
  if !(kError < .00002) then
   printks "VBAP moving=%g layout=%g block=%g sample=%g error=%g expected=(%g,%g) scalar=(%g,%g) array=(%g,%g) zak=(%g,%g)\n", 0, p4, p5, kBlock, kN, kError, kExpectedFirst, kExpectedSecond, kFirst, kSecond, kArrayFirst, kArraySecond, kZakFirst, kZakSecond
   exitnowk(-1)
  endif
  kN += 1
 od
 if kBlock == 1 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 12 then
  prints "VBAP cases did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Full blocks, onset only, onset and end in one block, and partial final block.
i 1 0 0.0078125 0 0
i 1 0.015625 0.0078125 0 3
i 1 0.0316162109375 0.0015869140625 0 0
i 1 0.0472412109375 0.0015869140625 0 3
i 1 0.0628662109375 0.0009765625 0 0
i 1 0.0784912109375 0.0009765625 0 3
i 1 0.0941162109375 0.0068359375 0 0
i 1 0.1097412109375 0.0068359375 0 3
i 1 0.125 0.0078125 1 0
i 1 0.1409912109375 0.0015869140625 1 0
i 1 0.1566162109375 0.0009765625 1 0
i 1 0.1722412109375 0.0068359375 1 0
i 99 .25 .001
e
</CsScore>
</CsoundSynthesizer>
