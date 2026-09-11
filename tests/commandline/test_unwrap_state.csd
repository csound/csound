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
  kInput[] fillarray 1, 4, -4, 0, 24*$M_PI+.25, -24*$M_PI-.25, 32*$M_PI, -32*$M_PI
  kExpected[] fillarray 1, 4-2*$M_PI, -4+2*$M_PI, 0, .25, -.25
  kCopy[] init 8
  kIndex = 0
  while kIndex < 8 do
    kCopy[kIndex] = kInput[kIndex]
    kIndex += 1
  od
  kOutput[] unwrap kInput
  kCopy unwrap kCopy
  kIndex = 0
  while kIndex < 8 do
    if kOutput[kIndex] != kCopy[kIndex] || !(abs(kOutput[kIndex]) <= $M_PI) then
      printks "unwrap: separate and in-place outputs differ at %g\n", 0, kIndex
      exitnowk -1
    endif
    if kIndex < 6 then
      if !(abs(kOutput[kIndex]-kExpected[kIndex]) <= .00002) then
        printks "unwrap: wrong wrapped phase at %g: %g\n", 0, kIndex, kOutput[kIndex]
        exitnowk -1
      endif
    endif
    kIndex += 1
  od
  if kInput[1] != 4 || kInput[2] != -4 then
    printks "unwrap changed its separate input\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 2
  kInput[] init 8
  kCopy[] init 8
  kCycle init 0
  kAge init 0
  if kCycle == 10 then
    kAge = 0
    reinit PHASES
  endif
  kIndex = 0
  while kIndex < 8 do
    kStep = (kIndex+1)*.3
    if kIndex % 2 == 0 then
      kStep = -kStep
    endif
    kPhase = kAge*kStep
    kInput[kIndex] = kPhase - 2*$M_PI*floor((kPhase+$M_PI)/(2*$M_PI))
    kCopy[kIndex] = kInput[kIndex]
    kIndex += 1
  od
PHASES:
  kOutput[] unwrap kInput, 1
  kCopy unwrap kCopy, 1
  rireturn
  kIndex = 0
  while kIndex < 8 do
    kStep = (kIndex+1)*.3
    if kIndex % 2 == 0 then
      kStep = -kStep
    endif
    kExpected = kAge*kStep
    if !(abs(kOutput[kIndex]-kExpected) <= .0001 && abs(kCopy[kIndex]-kExpected) <= .0001) then
      printks "unwrap: cycle %g index %g got %g / %g, expected %g\n", 0, kCycle, kIndex, kOutput[kIndex], kCopy[kIndex], kExpected
      exitnowk -1
    endif
    kIndex += 1
  od
  kAge += 1
  kCycle += 1
  if kCycle == 20 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  kInput[] init 0
  kWrapped[] unwrap kInput
  kUnwrapped[] unwrap kInput, 1
  if lenarray(kWrapped) != 0 || lenarray(kUnwrapped) != 0 then
    printks "unwrap changed an empty array's length\n", 0
    exitnowk -1
  endif
  gkChecks += 1
  turnoff
endin

instr 99
  if i(gkChecks) != 4 then
    prints "unwrap checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .1 .1
i 2 .3 .1
i 3 0 .1
i 99 .5 .01
e
</CsScore>
</CsoundSynthesizer>
