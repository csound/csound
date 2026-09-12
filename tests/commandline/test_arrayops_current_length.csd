<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
gkChecks init 0

instr 1
  iPowers[] fillarray 1,2,4,8
  iLogs[] log2 iPowers
  iAscending[] sorta iPowers
  iDescending[] sortd iPowers
  iIndex = 0
  while iIndex < 4 do
    if abs(iLogs[iIndex]-iIndex) > .00001 || iAscending[iIndex] != iPowers[iIndex] || iDescending[iIndex] != iPowers[3-iIndex] then
      prints "array init-time math or sort mismatch\n"
      exitnow(-1)
    endif
    iIndex += 1
  od

  kInput[] fillarray 4,3,2,1
  kExponents[] fillarray 2,2,2,2
  kCopy[] init 4
  kOtherCopy[] init 4
  kCycle timeinstk
  if kCycle == 2 then
    trim kInput, 2
    kInput[0] = 40
    kInput[1] = 30
  elseif kCycle == 4 then
    trim kInput, 0
  elseif kCycle == 6 then
    trim kInput, 4
    kInput[0] = 4
    kInput[1] = 3
    kInput[2] = 2
    kInput[3] = 1
  elseif kCycle == 7 then
    trim kInput, 1
    kInput[0] = 9
  endif
  kCount lenarray kInput
  kAscending[] sorta kInput
  kDescending[] sortd kInput
  kRoots[] sqrt kInput
  kPowers[] pow kInput,kExponents
  kScalarPowers[] pow kInput,2
  kLimited[] limit kInput,0,10
  kLogs[] log2 kInput
  kCopy = kInput
  kCopy sorta kCopy
  kOtherCopy = kExponents
  kOtherCopy pow kInput,kOtherCopy
  kDot dot kInput,kExponents

  kAscendingLength lenarray kAscending
  kDescendingLength lenarray kDescending
  kRootsLength lenarray kRoots
  kPowersLength lenarray kPowers
  kScalarPowersLength lenarray kScalarPowers
  kLimitedLength lenarray kLimited
  kLogsLength lenarray kLogs
  kCopyLength lenarray kCopy
  kOtherCopyLength lenarray kOtherCopy
  if kAscendingLength != kCount || kDescendingLength != kCount || kRootsLength != kCount || kPowersLength != kCount || kScalarPowersLength != kCount || kLimitedLength != kCount || kLogsLength != kCount || kCopyLength != kCount || kOtherCopyLength != kCount then
    printks "array output length mismatch in cycle %d: %d %d %d %d %d %d %d %d %d, expected %d\n",0,kCycle,kAscendingLength,kDescendingLength,kRootsLength,kPowersLength,kScalarPowersLength,kLimitedLength,kLogsLength,kCopyLength,kOtherCopyLength,kCount
    exitnowk(-1)
  endif
  kIndex = 0
  kExpectedDot = 0
  while kIndex < kCount do
    kValue = kInput[kIndex]
    kExpectedDot += 2*kValue
    if kAscending[kIndex] != kInput[kCount-1-kIndex] || kDescending[kIndex] != kValue || kCopy[kIndex] != kAscending[kIndex] || abs(kRoots[kIndex]-sqrt(kValue)) > .00001 || kPowers[kIndex] != kValue*kValue || kScalarPowers[kIndex] != kValue*kValue || kOtherCopy[kIndex] != kValue*kValue || kLimited[kIndex] != min(kValue,10) || abs(kLogs[kIndex]-log2(kValue)) > .00001 then
      printks "array result mismatch in cycle %d element %d\n",0,kCycle,kIndex
      exitnowk(-1)
    endif
    kIndex += 1
  od
  if kDot != kExpectedDot then
    printks "dot result mismatch\n",0
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if gkChecks != 1 then
    printks "array checks did not complete\n",0
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .125
i 99 .25 .01
</CsScore>
</CsoundSynthesizer>
