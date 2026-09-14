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
  setksmps p5
  kCycle init 0
  kSkip init 0
  kCycle += 1
  kInput = kCycle == 1 ? 1 : (kCycle%3-1)*.1
  kFrequency = sr/p5 * (kCycle < 5 ? .125 : .1875)
  kBandwidth = sr/p5 * (kCycle < 7 ? .0625 : .125)
  if kCycle == 9 then
    kSkip = p6
    reinit FILTERS
  endif
FILTERS:
  kPass resonk kInput, kFrequency, kBandwidth, p4, i(kSkip)
  kStop aresonk kInput, kFrequency, kBandwidth, p4, i(kSkip)
  kDefault aresonk kInput, kFrequency, kBandwidth
  kPeak aresonk kInput, kFrequency, kBandwidth, 1
  kCopy = kInput
  kCopy aresonk kCopy, kFrequency, kBandwidth, p4, i(kSkip)
  rireturn
  ; The notch and band-pass responses sum to the input (twice for scale 2).
  kExpected = kInput * (p4 == 2 ? 2 : 1)
  if abs(kPass+kStop-kExpected) > .00001 || abs(kCopy-kStop) > .00001 || abs(kDefault-kPeak) > .00001 then
    printks "aresonk mismatch: scale=%d ksmps=%d skip=%d cycle=%d pass=%g stop=%g expected sum=%g\n", 0, p4, p5, p6, kCycle, kPass, kStop, kExpected
    exitnowk(-1)
  endif
  if kCycle == 12 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  if gkChecks != 12 then
    printks "missing aresonk cases: %d\n", 0, gkChecks
    exitnowk(-1)
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .25 0 16 0
i 1 0 .25 1 16 0
i 1 0 .25 2 16 0
i 1 .25 .25 0 16 1
i 1 .25 .25 1 16 1
i 1 .25 .25 2 16 1
i 1 .5 .25 0 1 0
i 1 .5 .25 1 1 0
i 1 .5 .25 2 1 0
i 1 .75 .25 0 1 1
i 1 .75 .25 1 1 1
i 1 .75 .25 2 1 1
i 2 1 .01
</CsScore>
</CsoundSynthesizer>
