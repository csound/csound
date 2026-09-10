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
  setksmps 1
  kCycle init 0
  kCycle += 1
  aInput = kCycle == 1 ? 1 : 0
  aRefL, aRefH, aRefB svfilter aInput, 1000, 2
  if kCycle == 5 then
    reinit FILTER
  endif
FILTER:
  aL, aH, aB svfilter aInput, 1000, 2, 0, p4
  aDefaultL, aDefaultH, aDefaultB svfilter aInput, 1000, 2
  rireturn
  kL downsamp aL
  kH downsamp aH
  kB downsamp aB
  kDL downsamp aDefaultL
  kDH downsamp aDefaultH
  kDB downsamp aDefaultB
  kRefL downsamp aRefL
  kRefH downsamp aRefH
  kRefB downsamp aRefB
  kKeep = kCycle < 5 || p4 != 0 ? 1 : 0
  kDefaultKeep = kCycle < 5 ? 1 : 0
  kError = abs(kL - kKeep*kRefL) + abs(kH - kKeep*kRefH) + abs(kB - kKeep*kRefB)
  kError += abs(kDL - kDefaultKeep*kRefL) + abs(kDH - kDefaultKeep*kRefH)
  kError += abs(kDB - kDefaultKeep*kRefB)
  if !(kError <= .000001) then
    printks "svfilter iskip=%d failed on sample %d: error %g\n", 0, p4, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
  endif
endin

instr 2, 3, 4, 5
  aInput oscili .1, 500
  aCopy = aInput
  kFreq = 700
  kQ = 2
  aFreq = 700 + 100*aInput
  aQ = 2 + aInput
  if p1 == 2 then
    aRefL, aRefH, aRefB svfilter aInput, kFreq, kQ, p4
    aCopy, aH, aB svfilter aCopy, kFreq, kQ, p4
  elseif p1 == 3 then
    aRefL, aRefH, aRefB svfilter aInput, aFreq, kQ, p4
    aCopy, aH, aB svfilter aCopy, aFreq, kQ, p4
  elseif p1 == 4 then
    aRefL, aRefH, aRefB svfilter aInput, kFreq, aQ, p4
    aCopy, aH, aB svfilter aCopy, kFreq, aQ, p4
  else
    aRefL, aRefH, aRefB svfilter aInput, aFreq, aQ, p4
    aCopy, aH, aB svfilter aCopy, aFreq, aQ, p4
  endif
  aError = abs(aCopy - aRefL) + abs(aH - aRefH) + abs(aB - aRefB)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "svfilter input reuse differs in rate case %d: error %g\n", 0, p1, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 14 then
    prints "svfilter checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001 0
i 1 0 .001 1
i 2 .01 .004 0
i 3 .01 .004 0
i 4 .01 .004 0
i 5 .01 .004 0
i 2 .020625 .003 0
i 3 .020625 .003 0
i 4 .020625 .003 0
i 5 .020625 .003 0
i 2 .030625 .003 1
i 3 .030625 .003 1
i 4 .030625 .003 1
i 5 .030625 .003 1
i 99 .04 .002
</CsScore>
</CsoundSynthesizer>
