<CsTest>
description = "rms freezes its history and scale at zero cutoff"
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
instr Retained
  kSample init 0
  if kSample == 32 then
    reinit Detector
  endif
Detector:
  iCutoff = (i(kSample) < 32 ? sr/2 : 0)
  aInput = (kSample < 32 ? 1e-10 : 1e15)
  kLevel rms aInput, iCutoff, 1
  rireturn
  if kSample == 33 then
    if !(abs(kLevel/1e-10-1) < .00001) then
      printks "Zero cutoff lost retained RMS: %g\n", 0, kLevel
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr Fresh
  kSample init 0
  if kSample == 32 then
    reinit Detector
  endif
Detector:
  iCutoff = (i(kSample) < 32 ? 0 : sr/2)
  aInput = (kSample < 32 ? 1e15 : 1e-20)
  kLevel rms aInput, iCutoff, 1
  rireturn
  if kSample == 31 || kSample == 63 then
    kGood = (kSample == 31 ? kLevel == 0 : abs(kLevel/1e-20-1) < .00001)
    if kGood == 0 then
      printks "Zero cutoff changed empty detector scale: %g\n", 0, kLevel
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr Done
  if i(gkChecks) != 3 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Retained" 0 [64/48000]
i "Fresh" 0 [64/48000]
i "Done" [65/48000] [1/48000]
e
</CsScore>
</CsoundSynthesizer>
