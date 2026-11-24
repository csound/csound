<CsTest>
description = "RMS level matching preserves and resumes zero-cutoff history"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0
instr Compare
  kSample init 0
  if kSample == 32 then
    reinit Detectors
  endif
Detectors:
  if p4 == 0 then
    iCutoff = (i(kSample) < 32 ? sr/2 : 0)
    kSource = (kSample < 32 ? 1e-10 : 1e15)
    kReference = (kSample < 32 ? 2e-10 : 5e20)
  else
    iCutoff = (i(kSample) < 32 ? 0 : sr/2)
    kSource = (kSample < 32 ? 1e15 : 1e-20)
    kReference = 2*kSource
  endif
  aSource = kSource
  aReference = kReference
  kLevel rms aReference, iCutoff, 1
  aChain gain aSource, kLevel, iCutoff, 1
  aBalanced balance aSource, aReference, iCutoff, 1
  aPerSample balance2 aSource, aReference, iCutoff, 1
  rireturn
  if kSample == 63 then
    kChain downsamp aChain
    kBalanced downsamp aBalanced
    kPerSample downsamp aPerSample
    kExpected = 2*kSource
    if !(abs(kChain/kExpected-1) < .00001 && abs(kBalanced/kExpected-1) < .00001 && abs(kPerSample/kExpected-1) < .00001) then
      printks "RMS history case %g: chain %g, balance %g, balance2 %g, expected %g\n", 0, p4, kChain, kBalanced, kPerSample, kExpected
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr Done
  if i(gkChecks) != 2 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Compare" 0 [65/48000] 0
i "Compare" 0 [65/48000] 1
i "Done" [66/48000] [1/48000]
e
</CsScore>
</CsoundSynthesizer>
