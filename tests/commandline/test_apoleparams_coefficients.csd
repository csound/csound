<CsTest>
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 48
nchnls = 1
0dbfs = 1
instr 1
  kCoeffs[] fillarray -.5, .5
  kCycle init 0
  ; The first array sums to zero; the next two have the same sum.
  if kCycle == 1 then
    kCoeffs[0] = -1
    kCoeffs[1] = .5
  elseif kCycle == 2 then
    kCoeffs[0] = -.75
    kCoeffs[1] = .25
  elseif kCycle >= 3 then
    kCoeffs[0] = 0
    kCoeffs[1] = 0
  endif
  kParams[] apoleparams kCoeffs
  if kCycle < 3 then
    ; For 1 + c1*z^-1 + c2*z^-2, the pole radius is sqrt(c2).
    kRadius = sqrt(kCoeffs[1])
    kFrequency = cosinv(-kCoeffs[0]/(2*kRadius))*sr/(2*$M_PI)
    kBandwidth = -log(kRadius)*sr/$M_PI
  else
    kFrequency = 0
    kBandwidth = 0
  endif
  if !(abs(kParams[0]-kFrequency) < .1 && abs(kParams[1]-kBandwidth) < .1) then
    printks "apoleparams cycle %g: got [%g,%g], expected [%g,%g]\n", 0, kCycle, kParams[0], kParams[1], kFrequency, kBandwidth
    exitnowk(-1)
  endif
  ; A trailing zero coefficient adds a zero pole, which must be excluded.
  kOdd[] fillarray -1, .5, 0
  kOddParams[] apoleparams kOdd
  if !(abs(kOddParams[0]-6000) < .1 && \
       abs(kOddParams[1]+log(sqrt(.5))*sr/$M_PI) < .1 && kOddParams[2] == 0) then
    printks "apoleparams mishandled a zero pole\n", 0
    exitnowk(-1)
  endif
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .005
</CsScore>
</CsoundSynthesizer>
