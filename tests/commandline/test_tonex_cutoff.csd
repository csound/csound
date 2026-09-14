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
  kCycle init 0
  kCycle += 1
  kCutoff = kCycle == 1 ? 500 : (kCycle == 2 ? 1200 : 750)
  aInput oscili .5, 321
  aLow tonex aInput, kCutoff, p4
  aHigh atonex aInput, kCutoff, p4
  aRefLow tone aInput, kCutoff
  aRefHigh atone aInput, kCutoff
  if p4 == 3 then
    aRefLow tone aRefLow, kCutoff
    aRefLow tone aRefLow, kCutoff
    aRefHigh atone aRefHigh, kCutoff
    aRefHigh atone aRefHigh, kCutoff
  endif
  aError = abs(aLow-aRefLow) + abs(aHigh-aRefHigh)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "tonex/atonex control cutoff differs from cascade: %g\n", 0, kError
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 2
  aInput oscili .5, 321
  aMod oscili 500, 500
  aCutoff = 1000 + p5*aMod
  aLow tonex aInput, aCutoff, p4
  aHigh atonex aInput, aCutoff, p4
  aLowIn = aInput
  aHighIn = aInput
  aLowCut = aCutoff
  aHighCut = aCutoff
  aLowIn tonex aLowIn, aCutoff, p4
  aHighIn atonex aHighIn, aCutoff, p4
  aLowCut tonex aInput, aLowCut, p4
  aHighCut atonex aInput, aHighCut, p4
  aRefLow tone aInput, aCutoff
  aRefHigh atone aInput, aCutoff
  if p4 == 3 then
    aRefLow tone aRefLow, aCutoff
    aRefLow tone aRefLow, aCutoff
    aRefHigh atone aRefHigh, aCutoff
    aRefHigh atone aRefHigh, aCutoff
  endif
  aError = abs(aLow-aRefLow) + abs(aHigh-aRefHigh)
  aError += abs(aLowIn-aRefLow) + abs(aHighIn-aRefHigh)
  aError += abs(aLowCut-aRefLow) + abs(aHighCut-aRefHigh)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "tonex/atonex audio cutoff differs from cascade: %g\n", 0, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 3
  setksmps 1
  kCycle init 0
  kCycle += 1
  aInput = kCycle == 1 ? 1 : 0
  aCutoff = 500
  aRefLow tonex aInput, aCutoff, 3
  aRefHigh atonex aInput, aCutoff, 3
  if kCycle == 5 then
    reinit FILTER
  endif
FILTER:
  iSkip = i(kCycle) > 1 ? p4 : 0
  aLow tonex aInput, aCutoff, 3, iSkip
  aHigh atonex aInput, aCutoff, 3, iSkip
  rireturn
  kKeep = kCycle < 5 || p4 != 0 ? 1 : 0
  aError = abs(aLow-kKeep*aRefLow) + abs(aHigh-kKeep*aRefHigh)
  kError downsamp aError
  if !(kError <= .000001) then
    printks "tonex/atonex reinit iskip=%g failed: %g\n", 0, p4, kError
    exitnowk(-1)
  endif
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 10 then
    prints "tonex/atonex checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .006 1
i 1 .01 .006 3
i 2 .02 .006 1 0
i 2 .03 .006 3 0
i 2 .04 .006 1 1
i 2 .05 .006 3 1
i 2 .060625 .004 3 1
i 2 .070625 .001 1 1
i 3 .08 .002 0
i 3 .09 .002 1
i 99 .1 .002
</CsScore>
</CsoundSynthesizer>
