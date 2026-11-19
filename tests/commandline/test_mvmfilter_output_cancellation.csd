<CsTest>
description = "mvmfilter includes the saved correction in the output"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  kSample init 0
  aIn = (kSample == 0 ? 1 : (kSample == 1 ? 2^30 : (kSample == 2 ? -2^30 : 0)))
  aOut mvmfilter aIn, 0, 1e30
  kOut downsamp aOut
  if !(kSample != 2 || kOut == 1) then
    printks "mvmfilter lost the correction at the output\n", 0
    exitnowk -1
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008
</CsScore>
</CsoundSynthesizer>
