<CsTest>
description = "pareq retains steady gain near DC and Nyquist in float builds"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkCheckedBlocks init 0

instr CheckSteadyGain
  ; At DC, the low shelf must give its requested gain of 4.
  aConstant = 1
  aLow pareq aConstant, 1, 4, sqrt(.5), 1

  ; Mirror the low-shelf case around Nyquist: alternate +1 and -1.
  aAlternating oscils 1, sr/2, .25
  aHigh pareq aAlternating, sr/2-1, 4, sqrt(.5), 2

  ; A sine at the peaking filter's center frequency must also have gain 4.
  aSine oscils 1, 1, 0
  aPeak pareq aSine, 1, 4, sqrt(.5), 0

  ; Allow three seconds for the initial transient to decay, then check
  ; every sample over the remaining tenth of a second.
  if timeinsts() >= 3 then
    kIndex = 0
    while kIndex < ksmps do
      kLow vaget kIndex, aLow
      kHigh vaget kIndex, aHigh
      kAlternating vaget kIndex, aAlternating
      kPeak vaget kIndex, aPeak
      kSine vaget kIndex, aSine
      if !(abs(kLow-4) < .001) || !(abs(kHigh-4*kAlternating) < .001) || !(abs(kPeak-4*kSine) < .001) then
        printks "pareq gain: low %.12g (expected 4), high %.12g (expected %.12g), peak %.12g (expected %.12g)\n", 0, kLow, kHigh, 4*kAlternating, kPeak, 4*kSine
        exitnowk -1
      endif
      kIndex += 1
    od
    gkCheckedBlocks += 1
  endif
endin

instr CheckCompletion
  if i(gkCheckedBlocks) < 100 then
    prints "The steady-gain checks must run after the settling period\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckSteadyGain" 0 3.1
i "CheckCompletion" 3.2 .001
e
</CsScore>
</CsoundSynthesizer>
