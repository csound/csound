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
  aInput oscili .01, 370
  kCycle init 0
  kCycle += 1
  kBase = kCycle < 5 ? 700 : 1100
  kWidth = kCycle < 7 ? 100 : 150
  ; Corrected spacing starts at the base; legacy linear spacing starts at zero.
  if p7 == 0 then
    aBank resony aInput, kBase, kWidth, 1, 400, p4, p5
  else
    aBank resony aInput, kBase, kWidth, 1, 400, p4, p5, 0, p7
  endif
  kFirst = p7 == 0 && int(p4) != 0 ? 0 : kBase
  aRef reson aInput, kFirst, kWidth*kFirst/kBase, p5
  kError max_k abs(aBank-aRef), 1, 1
  ; The legacy zero-frequency recurrence grows without damping.
  kMagnitude max_k abs(aRef), 1, 1
  kTolerance = p7 == 0 ? .0001*(1+kMagnitude) : .00001
  if !(kError < kTolerance) then
    printks "resony single filter: mode %g scale %g error %g\n", 0, p4, p5, kError
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 2
  aInput oscili .01, 370
  aCopy = aInput
  kCycle init 0
  kCycle += 1
  kBase = kCycle < 5 ? 600 : 800
  kWidth = kCycle < 7 ? 80 : 120
  kSeparation = kCycle < 9 ? p6 : p6*.5
  kFirst = kBase
  if (p7 == 0 && int(p4) == 0) || (p7 == 1 && p4 == 0) then
    kSecond = kBase*2^(kSeparation/3)
    kThird = kBase*2^(2*kSeparation/3)
  elseif p7 == 0 then
    kFirst = 0
    kSecond = kBase*kSeparation/3
    kThird = kBase*2*kSeparation/3
  else
    kSecond = kBase+kSeparation/3
    kThird = kBase+2*kSeparation/3
  endif
  aFirst reson aInput, kFirst, kWidth*kFirst/kBase, p5
  aSecond reson aInput, kSecond, kWidth*kSecond/kBase, p5
  aThird reson aInput, kThird, kWidth*kThird/kBase, p5
  aRef = aFirst+aSecond+aThird
  if p7 == 0 then
    aBank resony aInput, kBase, kWidth, 3, kSeparation, p4, p5
    aCopy resony aCopy, kBase, kWidth, 3, kSeparation, p4, p5
  else
    aBank resony aInput, kBase, kWidth, 3, kSeparation, p4, p5, 0, p7
    aCopy resony aCopy, kBase, kWidth, 3, kSeparation, p4, p5, 0, p7
  endif
  kError max_k abs(aBank-aRef)+abs(aCopy-aBank), 1, 1
  kMagnitude max_k abs(aRef), 1, 1
  kTolerance = p7 == 0 ? .0001*(1+kMagnitude) : .00002
  if !(kError < kTolerance) then
    printks "resony bank: mode %g scale %g separation %g error %g\n", 0, p4, p5, kSeparation, kError
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 48 then
    prints "resony spacing checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Corrected mode: every nonzero isepmode selects linear spacing.
i 1 0 .03 0 0 0 1
i 1 0 .03 0 1 0 1
i 1 0 .03 0 2 0 1
i 1 0 .03 1 0 0 1
i 1 0 .03 1 1 0 1
i 1 0 .03 1 2 0 1
i 1 0 .03 .5 0 0 1
i 1 0 .03 .5 1 0 1
i 1 0 .03 .5 2 0 1
i 1 0 .03 -.5 0 0 1
i 1 0 .03 -.5 1 0 1
i 1 0 .03 -.5 2 0 1
; Compare the bank with explicit parallel resonators across partial blocks.
i 2 .040625 .029 1 0 900 1
i 2 .040625 .029 1 1 900 1
i 2 .040625 .029 1 2 900 1
i 2 .040625 .029 .5 0 -300 1
i 2 .040625 .029 .5 1 -300 1
i 2 .040625 .029 .5 2 -300 1
i 2 .040625 .029 0 0 1.5 1
i 2 .040625 .029 0 1 1.5 1
i 2 .040625 .029 0 2 1.5 1
i 2 .040625 .029 1 0 0 1
i 2 .040625 .029 1 1 0 1
i 2 .040625 .029 1 2 0 1
; Omitted icorrect keeps the legacy result, including fractional isepmode.
i 1 0 .03 0 0 0 0
i 1 0 .03 0 1 0 0
i 1 0 .03 0 2 0 0
i 1 0 .03 1 0 0 0
i 1 0 .03 1 1 0 0
i 1 0 .03 1 2 0 0
i 1 0 .03 .5 0 0 0
i 1 0 .03 .5 1 0 0
i 1 0 .03 .5 2 0 0
i 1 0 .03 -.5 0 0 0
i 1 0 .03 -.5 1 0 0
i 1 0 .03 -.5 2 0 0
i 2 .040625 .029 1 0 1.5 0
i 2 .040625 .029 1 1 1.5 0
i 2 .040625 .029 1 2 1.5 0
i 2 .040625 .029 .5 0 -300 0
i 2 .040625 .029 .5 1 -300 0
i 2 .040625 .029 .5 2 -300 0
i 2 .040625 .029 0 0 1.5 0
i 2 .040625 .029 0 1 1.5 0
i 2 .040625 .029 0 2 1.5 0
i 2 .040625 .029 1 0 0 0
i 2 .040625 .029 1 1 0 0
i 2 .040625 .029 1 2 0 0
i 99 .08 .002
</CsScore>
</CsoundSynthesizer>
