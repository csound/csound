<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gaOne init 1
gkChecks init 0

instr 1
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart - iStart%ksmps
  kCycle init 0
  kF = 32 + 16*kCycle
  kR = .1*kCycle
  aF = kF
  aR = kR
  aIn = gaOne*p4
  a0, a1 otafilter aIn, kF, kR, p5
  a2, a3 otafilter aIn, aF, kR, p5
  a4, a5 otafilter aIn, kF, aR, p5
  a6, a7 otafilter aIn, aF, aR, p5
  aReuseL = aIn
  aReuseL, aReuseTap otafilter aReuseL, aF, aR, p5
  aReuseT = aIn
  aReuseLow, aReuseT otafilter aReuseT, aF, aR, p5

  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle*ksmps + kN
    k0 vaget kN, a0
    k1 vaget kN, a1
    k2 vaget kN, a2
    k3 vaget kN, a3
    k4 vaget kN, a4
    k5 vaget kN, a5
    k6 vaget kN, a6
    k7 vaget kN, a7
    k8 vaget kN, aReuseL
    k9 vaget kN, aReuseTap
    k10 vaget kN, aReuseLow
    k11 vaget kN, aReuseT
    if kTime < iStart || kTime >= iEnd then
      kError = max(abs(k0), abs(k1), abs(k2), abs(k3), abs(k4), abs(k5), \
                   abs(k6), abs(k7), abs(k8), abs(k9), abs(k10), abs(k11))
    else
      kError = max(abs(k0-k2), abs(k0-k4), abs(k0-k6), abs(k0-k8), abs(k0-k10), \
                   abs(k1-k3), abs(k1-k5), abs(k1-k7), abs(k1-k9), abs(k1-k11))
      if kTime == iStart then
        ; At rest and with no resonance, the first samples have a closed form.
        kG = tan(3.141592653589793*kF/sr)
        kA = (kG-1)/(kG+1)
        kG = kG/(1+kG)
        kDrive = max(p5, 0)+1
        kInputStage = kG*tanh(limit(p4*kDrive, -4, 4))/kDrive
        kSecondStage = kG*tanh(limit(kInputStage*kDrive, -4, 4))/kDrive
        kExpectedTap = kSecondStage*(1-kA)
        kError = max(kError, abs(k0-p4*kG^4), abs(k1-kExpectedTap))
      endif
    endif
    if !(kError < .00005) then
      printks "otafilter mismatch: input %g, drive %g, sample %g, error %g\n", 0, p4, p5, kTime, kError
      exitnowk -1
    endif
    kN += 1
  od
  kCycle += 1
  gkChecks += 1
endin

instr 99
  if i(gkChecks) != 24 then
    prints "otafilter tests missed an audio block\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, a short note within one block, and notes ending mid-block.
i 1 0 .03125 .1 0
i 1 .0625 .03125 -.1 0
i 1 .125 .03125 2 9
i 1 .1875 .03125 -2 9
i 1 .2529296875 .0048828125 .1 0
i 1 .2841796875 .0048828125 -2 9
i 1 .3125 .0205078125 .1 0
i 1 .34375 .0205078125 -.1 0
i 1 .375 .0205078125 2 9
i 1 .40625 .0205078125 -2 9
i 1 .4404296875 .037109375 .1 0
i 1 .5029296875 .037109375 -2 9
i 99 .625 0
e
</CsScore>
</CsoundSynthesizer>
