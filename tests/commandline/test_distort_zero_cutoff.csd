<CsTest>
description = "distort preserves retained RMS when reinitialized with zero cutoff"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
giShape ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1
gkChecks init 0
instr Retained
  kSample init 0
  if kSample == 512 then
    reinit Freeze
  endif
Freeze:
  iCutoff = (i(kSample) < 512 ? sr/2 : 0)
  aInput = (kSample == 513 ? pow(2,80) : 1)
  aOutput distort aInput, 1, giShape, iCutoff, 1
  rireturn
  kOutput downsamp aOutput
  if kSample == 514 then
    if !(abs(kOutput-1) < .00001) then
      printks "Zero-cutoff reinit lost retained RMS: %g\n", 0, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr Fresh
  kSample init 0
  aInput init 1e24
  aOutput distort aInput, 1, giShape, 0
  kOutput downsamp aOutput
  if kSample == 1 then
    if !(abs(kOutput*32768-1) < .00001) then
      printks "Fresh zero-cutoff note lost the RMS floor: %g\n", 0, kOutput
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
i "Retained" 0 [516/8000]
i "Fresh" 0 [2/8000]
i "Done" [520/8000] [1/8000]
e
</CsScore>
</CsoundSynthesizer>
