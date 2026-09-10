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
  ; A single resonator always starts at the base frequency.
  aBank resony aInput, kBase, kWidth, 1, 400, p4, p5
  aRef reson aInput, kBase, kWidth, p5
  kError max_k abs(aBank-aRef), 1, 1
  if !(kError < .00001) then
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
  if p4 == 0 then
    kSecond = kBase*2^(kSeparation/3)
    kThird = kBase*2^(2*kSeparation/3)
  else
    kSecond = kBase+kSeparation/3
    kThird = kBase+2*kSeparation/3
  endif
  aFirst reson aInput, kBase, kWidth, p5
  aSecond reson aInput, kSecond, kWidth*kSecond/kBase, p5
  aThird reson aInput, kThird, kWidth*kThird/kBase, p5
  aRef = aFirst+aSecond+aThird
  aBank resony aInput, kBase, kWidth, 3, kSeparation, p4, p5
  aCopy resony aCopy, kBase, kWidth, 3, kSeparation, p4, p5
  kError max_k abs(aBank-aRef)+abs(aCopy-aBank), 1, 1
  if !(kError < .00002) then
    printks "resony bank: mode %g scale %g separation %g error %g\n", 0, p4, p5, kSeparation, kError
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 24 then
    prints "resony spacing checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Mode and normalization. Every nonzero mode value selects linear spacing.
i 1 0 .03 0 0
i 1 0 .03 0 1
i 1 0 .03 0 2
i 1 0 .03 1 0
i 1 0 .03 1 1
i 1 0 .03 1 2
i 1 0 .03 .5 0
i 1 0 .03 .5 1
i 1 0 .03 .5 2
i 1 0 .03 -.5 0
i 1 0 .03 -.5 1
i 1 0 .03 -.5 2
; Compare the bank with explicit parallel resonators across partial blocks.
i 2 .040625 .029 1 0 900
i 2 .040625 .029 1 1 900
i 2 .040625 .029 1 2 900
i 2 .040625 .029 .5 0 -300
i 2 .040625 .029 .5 1 -300
i 2 .040625 .029 .5 2 -300
i 2 .040625 .029 0 0 1.5
i 2 .040625 .029 0 1 1.5
i 2 .040625 .029 0 2 1.5
i 2 .040625 .029 1 0 0
i 2 .040625 .029 1 1 0
i 2 .040625 .029 1 2 0
i 99 .08 .002
</CsScore>
</CsoundSynthesizer>
