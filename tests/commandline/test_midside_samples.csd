<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gaLeft init 1
gaRight init .25
gkChecks init 0

instr 1
  iStart = int(p2*sr + .5)
  iEnd = int((p2+p3)*sr + .5)
  iBlock = iStart - (iStart % ksmps)
  kCycle init 0
  kWidth = p4 + kCycle*.25
  kW limit kWidth, 0, 1

  aMid, aSide st2ms gaLeft, gaRight
  aL = gaLeft
  aR = gaRight
  aL, aR st2ms aL, aR
  aSwapL = gaLeft
  aSwapR = gaRight
  aSwapR, aSwapL st2ms aSwapL, aSwapR

  aDecL, aDecR ms2st gaLeft, gaRight, kWidth
  aM = gaLeft
  aS = gaRight
  aM, aS ms2st aM, aS, kWidth
  aSwapM = gaLeft
  aSwapS = gaRight
  aSwapS, aSwapM ms2st aSwapM, aSwapS, kWidth
  aRoundL, aRoundR ms2st aMid, aSide, .5

  kN = 0
  while kN < ksmps do
    kTime = iBlock + kCycle*ksmps + kN
    kActive = (kTime >= iStart && kTime < iEnd ? 1 : 0)
    kMid = 1.25*kActive
    kSide = .75*kActive
    kLeft = (1-kW + .25*kW)*kActive
    kRight = (1-kW - .25*kW)*kActive
    k0 vaget kN, aMid
    k1 vaget kN, aSide
    k2 vaget kN, aL
    k3 vaget kN, aR
    k4 vaget kN, aSwapR
    k5 vaget kN, aSwapL
    k6 vaget kN, aDecL
    k7 vaget kN, aDecR
    k8 vaget kN, aM
    k9 vaget kN, aS
    k10 vaget kN, aSwapS
    k11 vaget kN, aSwapM
    k12 vaget kN, aRoundL
    k13 vaget kN, aRoundR
    if k0 != kMid || k1 != kSide || k2 != kMid || k3 != kSide || \
       k4 != kMid || k5 != kSide || k6 != kLeft || k7 != kRight || \
       k8 != kLeft || k9 != kRight || k10 != kLeft || k11 != kRight || \
       k12 != kActive || k13 != .25*kActive then
      printks "mid/side mismatch at sample %g, width %g\n", 0, kTime, kWidth
      exitnowk -1
    endif
    kN += 1
  od
  kCycle += 1
  gkChecks += 1
endin

instr 99
  if i(gkChecks) != 12 then
    prints "mid/side tests did not process every expected block\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full blocks, a short note inside one block, and a partial first and last block.
i 1 0 .03125 -1
i 1 .0625 .03125 0
i 1 .125 .03125 .5
i 1 .1875 .03125 1
i 1 .2529296875 .0048828125 .25
i 1 .3154296875 .037109375 .25
i 99 .375 0
e
</CsScore>
</CsoundSynthesizer>
