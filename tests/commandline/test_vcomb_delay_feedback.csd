<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Reference, aa, aaki
  setksmps 1
  aInput, aDelay, kRvt, iMaximum xin
  iSize = int(iMaximum)
  kComb[] init iSize
  kAll[] init iSize
  kWrite init 0
  kInput downsamp aInput
  kRequested downsamp aDelay
  if kRequested < 1 then
    kDelay = 1
  elseif kRequested >= iSize then
    kDelay = iSize
  else
    kDelay = int(kRequested)
  endif
  kRead = (kWrite-kDelay+iSize) % iSize
  if kRvt == 0 then
    kGain = 0
  else
    kGain = .001^(kDelay/(sr*kRvt))
  endif
  kCombOut = kComb[kRead]
  kAllRead = kAll[kRead]
  kComb[kWrite] = kInput+kGain*kCombOut
  kAll[kWrite] = kInput+kGain*kAllRead
  kAllOut = kAllRead-kGain*kAll[kWrite]
  kWrite = (kWrite+1) % iSize
  aComb = kCombOut
  aAll = kAllOut
  xout aComb, aAll
endop

instr 1
  kCycle init 0
  kProcessed init 0
  kMaximum init p5
  kCycle += 1
  kRvt = (p9 != 0 ? 0 : (kCycle < 7 ? .01 : .02))
  kDelay = p6
  aPhase phasor 1193
  aInput = .05+.1*aPhase
  aDelay = kDelay+p7*(aPhase*32-8)
  aFixedDelay = kDelay
  if p4 == 0 then
    kLoop = kDelay/sr
    aLoop = aDelay/sr
  else
    kLoop = kDelay
    aLoop = aDelay
  endif
  if p8 > 0 && kCycle == 5 then
    kMaximum = p8
    reinit FILTERS
  endif
FILTERS:
  iMaximum = i(kMaximum)
  iLimit = (p4 == 0 ? iMaximum/sr : iMaximum)
  aRefComb, aRefAll Reference aInput, aFixedDelay, kRvt, iMaximum
  aRefCombAudio, aRefAllAudio Reference aInput, aDelay, kRvt, iMaximum
  aComb vcomb aInput, kRvt, kLoop, iLimit, 0, p4
  aAll valpass aInput, kRvt, kLoop, iLimit, 0, p4
  aCombAudio vcomb aInput, kRvt, aLoop, iLimit, 0, p4
  aAllAudio valpass aInput, kRvt, aLoop, iLimit, 0, p4
  aCopy = aInput
  aCopy vcomb aCopy, kRvt, aLoop, iLimit, 0, p4
  rireturn
  kIndex = 0
  while kIndex < ksmps do
    kComb vaget kIndex, aComb
    kAll vaget kIndex, aAll
    kCombAudio vaget kIndex, aCombAudio
    kAllAudio vaget kIndex, aAllAudio
    kCopy vaget kIndex, aCopy
    kRefComb vaget kIndex, aRefComb
    kRefAll vaget kIndex, aRefAll
    kRefCombAudio vaget kIndex, aRefCombAudio
    kRefAllAudio vaget kIndex, aRefAllAudio
    kError = abs(kComb-kRefComb)+abs(kAll-kRefAll)
    kError += abs(kCombAudio-kRefCombAudio)+abs(kAllAudio-kRefAllAudio)+abs(kCopy-kRefCombAudio)
    if !(kError <= .00005) then
      printks "vcomb/valpass units %g delay %g cycle %g sample %g error %g\n", 0, p4, p6, kCycle, kIndex, kError
      exitnowk -1
    endif
    kIndex += 1
  od
  kOffset offsetsmps
  kEarly earlysmps
  kProcessed += ksmps-kOffset-kEarly
  if kProcessed == int(p3*sr) then
    gkChecks += 1
  endif
endin

; Skipping initialization with the same size must retain delay history.
instr 2
  kCycle init 0
  aInput oscili .1, 437
  aDelay = 20
  aRefComb, aRefAll Reference aInput, aDelay, .01, 8
  if kCycle == 5 then
    reinit FILTERS
  endif
FILTERS:
  aComb vcomb aInput, .01, 20, 8, 1, 1
  aAll valpass aInput, .01, 20, 8, 1, 1
  rireturn
  aError = abs(aComb-aRefComb)+abs(aAll-aRefAll)
  kError max_k aError, 1, 1
  if !(kError <= .00001) then
    printks "vcomb/valpass lost history on skipped init\n", 0
    exitnowk -1
  endif
  kCycle += 1
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 22 then
    prints "vcomb/valpass checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Units, maximum in samples, requested samples, modulation, new maximum, zero rvt.
i 1 0 .0625 0 8.75 4.75 0 0 0
i 1 0 .0625 1 8.75 4.75 0 0 0
i 1 0 .0625 0 8 20 0 0 0
i 1 0 .0625 1 8 20 0 0 0
i 1 0 .0625 0 8 0 0 0 0
i 1 0 .0625 1 8 0 0 0 0
i 1 0 .0625 0 8 -3 0 0 0
i 1 0 .0625 1 8 -3 0 0 0
i 1 0 .0625 0 8 1e20 0 0 0
i 1 0 .0625 1 8 -1e20 0 0 0
i 1 0 .0625 0 8 4 1 0 0
i 1 0 .0625 1 8 4 1 0 0
i 1 0 .0625 1 1 20 0 0 0
i 1 0 .0625 1 8 20 0 0 1
i 1 0 .0625 0 8 4 1 0 1
i 1 0 .0625 1 8 4 1 0 1
i 1 0 .0625 0 16 20 1 8 0
i 1 0 .0625 1 8 20 1 16 0
i 1 .0006103515625 .0130615234375 0 8 20 1 0 0
i 1 .0006103515625 .0013427734375 1 8 4 1 0 0
i 1 .125 .0625 1 8 20 0 0 0
i 2 .25 .0625
i 99 .375 .01
e
</CsScore>
</CsoundSynthesizer>
