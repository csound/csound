<CsTest>
description = "rms and gain agree with balance through a low-cutoff pulse decay"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1
gkChecks init 0
instr Compare
  aSource = 1
  aReference mpulse 1, 0
  kReference rms aReference, p4
  aChain gain aSource, kReference, p4
  aBalanced balance aSource, aReference, p4
  aPerSample balance2 aSource, aReference, p4
  kBlock init 0
  kBlock += 1
  if kBlock == 450000 then
    kChain vaget ksmps-1, aChain
    kBalanced vaget ksmps-1, aBalanced
    kPerSample vaget ksmps-1, aPerSample
    ; At these cutoffs c1 is 2*pi*f/sr to much better than test tolerance.
    kExpected = sqrt(2*$M_PI*p4/sr)*exp(-$M_PI*p4*600)/sqrt(1-exp(-2*$M_PI*p4*600))
    if !(abs(kChain/kBalanced-1) < .00001 && abs(kChain/kExpected-1) < .00001 && abs(kPerSample/kExpected-1) < .00001) then
      printks "Pulse decay cutoff %g: rms/gain %g, balance %g, balance2 %g, expected %g\n", 0, p4, kChain, kBalanced, kPerSample, kExpected
      exitnowk -1
    endif
    gkChecks += 1
  endif
endin
instr Done
  if i(gkChecks) != 1 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Compare" 0 600.01 .0001
i "Done" 600.02 .01
e
</CsScore>
</CsoundSynthesizer>
