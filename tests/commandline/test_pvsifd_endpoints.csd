<CsTest>
description = "pvsifd endpoint magnitudes and phases follow input polarity and gain"

[expect]
exit = 0
</CsTest>
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

instr CheckEndpoint
 ; p4 is input polarity; p5 chooses DC or Nyquist; p6 is analysis gain.
 ; Both outputs must report a nonnegative magnitude. A negative coefficient
 ; has phase pi, and a zero coefficient has phase zero.
 aPhase phasor sr/2
 if p5 == 0 then
  aInput = p4 * .25
 else
  aInput = p4 * .25 * (1-4*aPhase)
 endif
 fFrequency, fPhase pvsifd aInput, 64, 16, p7, p6
 kFrequency[] init 66
 kPhase[] init 66
 kFrame pvs2array kFrequency, fFrequency
 kPhaseFrame pvs2array kPhase, fPhase
 kCycle init 0
 kCycle += 1
 if kCycle == 12 then
  iBin = (p5 == 0 ? 0 : 32)
  iMagnitude = abs(p4*p6)*.25
  iPhase = (p4*p6 < 0 ? 3.141592653589793 : 0)
  iFrequency = (p5 == 0 ? 0 : sr/2)
  kError = abs(kFrequency[2*iBin]-iMagnitude) + abs(kPhase[2*iBin]-iMagnitude) + abs(kPhase[2*iBin+1]-iPhase) + abs(kFrequency[2*iBin+1]-iFrequency)
  if !(kError < 1e-5) then
   printks "pvsifd endpoint: polarity=%g bin=%g gain=%g magnitude=%g phase=%g error=%g\n", 0, p4, iBin, p6, kFrequency[2*iBin], kPhase[2*iBin+1], kError
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 10 then
  prints "pvsifd endpoint checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Polarity, DC/Nyquist, gain, window (Hamming=0, Hann=1).
i "CheckEndpoint" 0 .04  1 0  1 0
i "CheckEndpoint" 0 .04 -1 0  1 0
i "CheckEndpoint" 0 .04  1 1  1 0
i "CheckEndpoint" 0 .04 -1 1  1 0
i "CheckEndpoint" 0 .04  1 0  1 1
i "CheckEndpoint" 0 .04 -1 0  1 1
i "CheckEndpoint" 0 .04  1 1 -.5 1
i "CheckEndpoint" 0 .04 -1 1 -.5 1
i "CheckEndpoint" 0 .04 -1 0  0 1
i "CheckEndpoint" 0 .04 -1 1  0 1
i "CheckResults" .05 .01
e
</CsScore>
</CsoundSynthesizer>
