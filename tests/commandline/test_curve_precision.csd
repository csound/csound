<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 iSteepness[] fillarray 1, 1.0000001, 1.0000000000000002, 2, 16, 1e10, 1e30, .5, 1e-20
 kCycle init 0
 kSteepness = iSteepness[int(kCycle/5)]
 kIndex = (kCycle%5)*.25
 kExp expcurve kIndex, kSteepness
 kLog logcurve kIndex, kSteepness
 kRoundTrip logcurve kExp, kSteepness
 kReverse expcurve kLog, kSteepness
 if kIndex == 0 || kIndex == 1 then
  if kExp != kIndex || kLog != kIndex then
   printks "curve endpoint steepness=%g index=%g exp=%g log=%g\n", 0, kSteepness, kIndex, kExp, kLog
   exitnowk(-1)
  endif
 elseif kSteepness >= 1 then
  ; The normalized exponential and logarithmic curves are inverses.
  if !(abs(kRoundTrip-kIndex) < .000003) || !(abs(kReverse-kIndex) < .000003) then
   printks "curve inverse steepness=%.17g index=%g forward=%g reverse=%g\n", 0, kSteepness, kIndex, kRoundTrip, kReverse
   exitnowk(-1)
  endif
  if kSteepness-1 < .000001 then
   if !(abs(kExp-kIndex) < .000001) || !(abs(kLog-kIndex) < .000001) then
    printks "curve near-linear limit failed\n", 0
    exitnowk(-1)
   endif
  endif
  if kSteepness == 16 && kIndex == .5 then
   if !(abs(kExp-.2) < .000001) then
    printks "curve known value failed\n", 0
    exitnowk(-1)
   endif
  endif
 else
  ; Preserve expcurve's linear fallback and logcurve's existing curve below 1.
  kExpectedLog = log((1-kIndex)+kIndex*kSteepness)/log(kSteepness)
  if kExp != kIndex || !(abs(kLog-kExpectedLog) < .000001) then
   printks "curve below-one steepness=%g index=%g exp=%g log=%g\n", 0, kSteepness, kIndex, kExp, kLog
   exitnowk(-1)
  endif
 endif
 kCycle += 1
 gkChecks += 1
endin

instr 99
 if i(gkChecks) != 45 then
  prints "curve checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .087890625
i 99 .1 .001
e
</CsScore>
</CsoundSynthesizer>
