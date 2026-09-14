<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kCount init 0
 kOversample init p6
 kLp init 0
 kBpd init 0
 kLpd init 0
 kCount += 1
 aInput = .1
 aFreq = 500
 aQ = p5
 if kCount == 20 || kCount == 40 || kCount == 60 then
  if kCount == 20 then
   kOversample = 6
  elseif kCount == 40 then
   kOversample = 1
  else
   kOversample = 0
  endif
  if p4 == 0 then
   kLp = 0
   kBpd = 0
   kLpd = 0
  endif
  reinit FILTER
 endif
FILTER:
 iOversample = i(kOversample)
 if p7 == 0 then
  aHp, aLp, aBp, aBr statevar aInput, 500, p5, iOversample, p4
 elseif p7 == 1 then
  aHp, aLp, aBp, aBr statevar aInput, aFreq, p5, iOversample, p4
 elseif p7 == 2 then
  aHp, aLp, aBp, aBr statevar aInput, 500, aQ, iOversample, p4
 else
  aHp, aLp, aBp, aBr statevar aInput, aFreq, aQ, iOversample, p4
 endif
 rireturn

 ; Keep the reference history independently across oversampling changes.
 kSteps = (kOversample <= 0 ? 3 : max(1, int(kOversample)))
 kF = 2*sin(500*$M_PI/sr/kSteps)
 kQ = max(1/p5, (2-kF)*.05/kSteps)
 kStep = 0
 while kStep < kSteps do
  kHp = .1-kQ*kBpd-kLp
  kBp = kHp*kF+kBpd
  kLp = kBpd*kF+kLpd
  kBr = kLp+kHp
  kBpd = kBp
  kLpd = kLp
  kStep += 1
 od
 kActualHp downsamp aHp
 kActualLp downsamp aLp
 kActualBp downsamp aBp
 kActualBr downsamp aBr
 kError = abs(kActualHp-kHp)+abs(kActualLp-kLp)+abs(kActualBp-kBp)+abs(kActualBr-kBr)
 if !(kError < .00004) then
  printks "statevar istor=%g Q=%g rates=%g sample=%g oversampling=%g error=%g\n", 0, p4, p5, p7, kCount, kOversample, kError
  exitnowk(-1)
 endif
 if kCount == 80 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 12 then
  prints "statevar checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Preserve and reset history, all parameter rate combinations, and Q limiting.
i 1 0 .0125 1 1 3 0
i 1 0 .0125 1 1 3 1
i 1 0 .0125 1 1 3 2
i 1 0 .0125 1 1 3 3
i 1 0 .0125 1 100 3 0
i 1 0 .0125 1 100 3 3
i 1 0 .0125 0 1 3 0
i 1 0 .0125 0 100 3 3
; Sub-unit counts take one step; other fractional counts still truncate.
i 1 0 .0125 1 1 .5 0
i 1 0 .0125 1 1 3.75 0
i 1 0 .0125 1 1 0 0
i 1 0 .0125 1 1 -1 0
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
