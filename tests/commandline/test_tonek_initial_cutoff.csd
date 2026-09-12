<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  setksmps p4
  ; At one eighth of the processing rate, the pole is 0.4733977176.
  iPole = .4733977176348845
  iCutoff = sr / p4 / 8
  kChanged init 0
  kChanged = iCutoff
  kLow tonek 1, iCutoff
  kHigh atonek 1, iCutoff
  kUpdatedLow tonek 1, kChanged
  kUpdatedHigh atonek 1, kChanged
  ; The shared initializer must still use sr for audio outputs.
  aStep = 1
  aLow tone aStep, sr/8
  aHigh atone aStep, sr/8
  kCycle timeinstk
  kTail = iPole ^ kCycle
  kAudioTail = iPole ^ (kCycle * p4)
  kAudioLow vaget p4-1, aLow
  kAudioHigh vaget p4-1, aHigh
  if abs(kLow - (1-kTail)) > .00001 || abs(kHigh - kTail) > .00001 || abs(kLow-kUpdatedLow) > .00001 || abs(kHigh-kUpdatedHigh) > .00001 then
    printks "tonek/atonek initial cutoff mismatch: ksmps=%d cycle=%d low=%g high=%g\n", 0, p4, kCycle, kLow, kHigh
    exitnowk(-1)
  endif
  if abs(kAudioLow - (1-kAudioTail)) > .00001 || abs(kAudioHigh-kAudioTail) > .00001 then
    printks "tone/atone audio initialization changed\n", 0
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  kCycle init 0
  kSkip init 0
  kCycle += 1
  if kCycle == 5 then
    kSkip = p4
    reinit FILTER
  endif
FILTER:
  kLow tonek 1, 8, i(kSkip)
  kHigh atonek 1, 8, i(kSkip)
  rireturn
  kAge = kCycle
  if p4 == 0 && kCycle >= 5 then
    kAge = kCycle - 4
  endif
  kTail = .4733977176348845 ^ kAge
  if abs(kLow-(1-kTail)) > .00001 || abs(kHigh-kTail) > .00001 then
    printks "tonek/atonek reinit mismatch: skip=%d cycle=%d low=%g high=%g\n", 0, p4, kCycle, kLow, kHigh
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  if gkChecks != 5 then
    printks "missing tonek/atonek cases: %d\n", 0, gkChecks
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .25 1
i 1 .25 .25 4
i 1 .5 .25 16
i 2 .75 .25 0
i 2 1 .25 1
i 3 1.25 .01
</CsScore>
</CsoundSynthesizer>
