<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
giShape ftgen 1, 0, 17, -7, -1, 16, 1
giNorm ftgen 2, 0, 17, -7, .5, 16, .5
gkChecks init 0
gaOne init 1

instr 1
  iShape = (p5 == 0 ? 0 : giShape)
  iNorm = (p5 == 2 ? giNorm : 0)
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart - iStart%ksmps
  kCycle init 0
  kF = 32 + kCycle*16
  kQ = .25 + kCycle*.5
  kDrive = p4
  aIn = gaOne*p6
  aF = kF
  aQ = kQ
  aH0, aL0, aB0, aR0 svn aIn, kF, kQ, kDrive, iShape, iNorm
  aH1, aL1, aB1, aR1 svn aIn, aF, kQ, kDrive, iShape, iNorm
  aH2, aL2, aB2, aR2 svn aIn, kF, aQ, kDrive, iShape, iNorm
  aH3, aL3, aB3, aR3 svn aIn, aF, aQ, kDrive, iShape, iNorm
  aReuse = aIn
  aReuse, aReuseL, aReuseB, aReuseR svn aReuse, aF, aQ, kDrive, iShape, iNorm

  ; Independent state-variable recurrence, using tanh or the linear custom map.
  kS0 init 0
  kS1 init 0
  kW = tan(3.141592653589793*kF/sr)
  kD = 1/max(kQ, .5)
  kFac = 1/(1 + kW*kD + kW*kW)
  kKn = kDrive
  if kKn > 0 then
    if p5 == 2 then
      kKn = min(kKn, 1)
      kGain = .5
    else
      kKn *= (p5 == 0 ? 8 : 2)
      kGain = 1/kKn
    endif
  endif
  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle*ksmps + kN
    if kTime >= iStart && kTime < iEnd then
      kH = (p6 - (kD+kW)*kS0 - kS1)*kFac
      kV = kH
      if kKn > 0 then
        if p5 == 0 then
          kV = tanh(limit(kH*kKn, -4, 4))*kGain
        else
          kV = limit(kH*kKn, -1, 1)*kGain
        endif
      endif
      kU = kW*kV
      kB = kU + kS0
      kS0 = kB + kU
      kV = kB
      if kKn > 0 then
        if p5 == 0 then
          kV = tanh(limit(kB*kKn, -4, 4))*kGain
        else
          kV = limit(kB*kKn, -1, 1)*kGain
        endif
      endif
      kU = kW*kV
      kL = kU + kS1
      kS1 = kL + kU
      kR = kH + kL
    else
      kH = 0
      kL = 0
      kB = 0
      kR = 0
    endif
    kH0 vaget kN, aH0
    kL0 vaget kN, aL0
    kB0 vaget kN, aB0
    kR0 vaget kN, aR0
    kH1 vaget kN, aH1
    kL1 vaget kN, aL1
    kB1 vaget kN, aB1
    kR1 vaget kN, aR1
    kH2 vaget kN, aH2
    kL2 vaget kN, aL2
    kB2 vaget kN, aB2
    kR2 vaget kN, aR2
    kH3 vaget kN, aH3
    kL3 vaget kN, aL3
    kB3 vaget kN, aB3
    kR3 vaget kN, aR3
    kRH vaget kN, aReuse
    kRL vaget kN, aReuseL
    kRB vaget kN, aReuseB
    kRR vaget kN, aReuseR
    kError = max(abs(kH0-kH), abs(kL0-kL), abs(kB0-kB), abs(kR0-kR))
    kError = max(kError, abs(kH1-kH), abs(kL1-kL), abs(kB1-kB), abs(kR1-kR))
    kError = max(kError, abs(kH2-kH), abs(kL2-kL), abs(kB2-kB), abs(kR2-kR))
    kError = max(kError, abs(kH3-kH), abs(kL3-kL), abs(kB3-kB), abs(kR3-kR))
    kError = max(kError, abs(kRH-kH), abs(kRL-kL), abs(kRB-kB), abs(kRR-kR))
    if !(kError < .0002) then
      printks "svn mismatch: drive %g, map %g, input %g, sample %g, error %g\n", 0, p4, p5, p6, kTime, kError
      printks "reference %g %g %g %g; kk %g %g %g %g; aa %g %g %g %g\n", 0, kH, kL, kB, kR, kH0, kL0, kB0, kR0, kH3, kL3, kB3, kR3
      exitnowk -1
    endif
    kN += 1
  od
  kCycle += 1
  gkChecks += 1
endin

instr 99
  if i(gkChecks) != 120 then
    prints "svn tests did not process all expected blocks\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125 0 0 -2
i 1 0 .03125 0 0 0.5
i 1 0 .03125 0.125 0 -2
i 1 0 .03125 0.125 0 0.5
i 1 0 .03125 1 0 -2
i 1 0 .03125 1 0 0.5
i 1 0 .03125 4 0 -2
i 1 0 .03125 4 0 0.5
i 1 0 .03125 0 1 -2
i 1 0 .03125 0 1 0.5
i 1 0 .03125 0.125 1 -2
i 1 0 .03125 0.125 1 0.5
i 1 0 .03125 1 1 -2
i 1 0 .03125 1 1 0.5
i 1 0 .03125 4 1 -2
i 1 0 .03125 4 1 0.5
i 1 0 .03125 0 2 -2
i 1 0 .03125 0 2 0.5
i 1 0 .03125 0.125 2 -2
i 1 0 .03125 0.125 2 0.5
i 1 0 .03125 1 2 -2
i 1 0 .03125 1 2 0.5
i 1 0 .03125 4 2 -2
i 1 0 .03125 4 2 0.5
i 1 0.1279296875 .03125 0 0 -2
i 1 0.1279296875 .03125 0 0 0.5
i 1 0.1279296875 .03125 0.125 0 -2
i 1 0.1279296875 .03125 0.125 0 0.5
i 1 0.1279296875 .03125 1 0 -2
i 1 0.1279296875 .03125 1 0 0.5
i 1 0.1279296875 .03125 4 0 -2
i 1 0.1279296875 .03125 4 0 0.5
i 1 0.1279296875 .03125 0 1 -2
i 1 0.1279296875 .03125 0 1 0.5
i 1 0.1279296875 .03125 0.125 1 -2
i 1 0.1279296875 .03125 0.125 1 0.5
i 1 0.1279296875 .03125 1 1 -2
i 1 0.1279296875 .03125 1 1 0.5
i 1 0.1279296875 .03125 4 1 -2
i 1 0.1279296875 .03125 4 1 0.5
i 1 0.1279296875 .03125 0 2 -2
i 1 0.1279296875 .03125 0 2 0.5
i 1 0.1279296875 .03125 0.125 2 -2
i 1 0.1279296875 .03125 0.125 2 0.5
i 1 0.1279296875 .03125 1 2 -2
i 1 0.1279296875 .03125 1 2 0.5
i 1 0.1279296875 .03125 4 2 -2
i 1 0.1279296875 .03125 4 2 0.5
i 99 .25 0
e
</CsScore>
</CsoundSynthesizer>
