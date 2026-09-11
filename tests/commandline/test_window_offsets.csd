<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kInput[] init 8
  kCopy[] init 8
  kCycle init 0
  kIndex = 0
  while kIndex < 8 do
    kInput[kIndex] = kIndex+1
    kCopy[kIndex] = kInput[kIndex]
    kIndex += 1
  od
  kOffset = p5
  kShift = p6
  if p5 < 1e15 then
    kOffset += kCycle*9
    kShift = (p6+kCycle) % 8
  endif
  kOutput[] window kInput, kOffset, p4
  kCopy window kCopy, kOffset, p4
  kIndex = 0
  while kIndex < 8 do
    kPhase = (kIndex-kShift)*2*$M_PI/8
    if p4 == 0 then
      kWeight = .54-.46*cos(kPhase)
    else
      kWeight = .5-.5*cos(kPhase)
    endif
    kExpected = (kIndex+1)*kWeight
    if !(abs(kOutput[kIndex]-kExpected) <= .00001 && abs(kCopy[kIndex]-kExpected) <= .00001) then
      printks "window type %g offset %g index %g: got %g / %g, expected %g\n", 0, p4, kOffset, kIndex, kOutput[kIndex], kCopy[kIndex], kExpected
      exitnowk -1
    endif
    if kInput[kIndex] != kIndex+1 then
      printks "window changed its separate input\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
  kCycle += 1
  if kCycle == 16 then
    gkChecks += 1
    turnoff
  endif
endin

; Rebuild the coefficients and output when reinit changes the input length.
instr 2
  kCycle init 0
  kLength init 8
  if kCycle == 2 then
    kLength = 3
    reinit WINDOW
  elseif kCycle == 4 then
    kLength = 13
    reinit WINDOW
  endif
WINDOW:
  kInput[] init i(kLength)
  kCopy[] init i(kLength)
  kIndex = 0
  while kIndex < kLength do
    kInput[kIndex] = kIndex+1
    kCopy[kIndex] = kInput[kIndex]
    kIndex += 1
  od
  kOutput[] window kInput, 4, p4
  kCopy window kCopy, 4, p4
  rireturn
  kSize lenarray kOutput
  if kSize != kLength then
    printks "window retained its old output length\n", 0
    exitnowk -1
  endif
  kIndex = 0
  while kIndex < kLength do
    kPhase = (kIndex-4)*2*$M_PI/kLength
    if p4 == 0 then
      kWeight = .54-.46*cos(kPhase)
    else
      kWeight = .5-.5*cos(kPhase)
    endif
    kExpected = (kIndex+1)*kWeight
    if !(abs(kOutput[kIndex]-kExpected) <= .00002 && abs(kCopy[kIndex]-kExpected) <= .00002) then
      printks "window reinit type %g length %g index %g: got %g / %g, expected %g\n", 0, p4, kLength, kIndex, kOutput[kIndex], kCopy[kIndex], kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kCycle += 1
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  kInput[] init 1
  kInput[0] = 1
  kHamming[] window kInput, 1e20, 0
  kHann[] window kInput
  if !(abs(kHamming[0]-.08) <= .000001 && kHann[0] == 0) then
    printks "window failed for a single element\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 4
  kInput[] init 0
  kHamming[] window kInput, 1e20, 0
  kHann[] window kInput
  if lenarray(kHamming) != 0 || lenarray(kHann) != 0 then
    printks "window changed an empty array's length\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 99
  if i(gkChecks) != 18 then
    prints "window checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; type, offset, equivalent integer offset
i 1 0 .1 0 0 0
i 1 0 .1 1 0 0
i 1 0 .1 0 1.75 1
i 1 0 .1 1 1.75 1
i 1 0 .1 0 8 0
i 1 0 .1 1 8 0
i 1 0 .1 0 17.75 1
i 1 0 .1 1 17.75 1
i 1 0 .1 0 1e20 0
i 1 0 .1 1 1e20 0
i 1 0 .1 0 7.75 7
i 1 0 .1 1 7.75 7
i 2 .1 .1 0
i 2 .1 .1 1
i 2 .3 .1 0
i 2 .3 .1 1
i 3 .4 .01
i 4 .4 .01
i 99 .5 .01
e
</CsScore>
</CsoundSynthesizer>
