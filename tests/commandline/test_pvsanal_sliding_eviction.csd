<CsTest>
description = "sliding pvsanal keeps a quiet sample until it leaves"

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
  kInput = (kSample == 0 ? 1 : (kSample == 1 ? 2^(-p4) : 0))
  aInput = kInput
  fAnalysis pvsanal aInput, 4, 1, 4, 9
  aMagnitude, aFrequency pvsbin fAnalysis, 0
  kMagnitude downsamp aMagnitude
  if kSample >= 4 then
    if !(kMagnitude == (kSample == 4 ? 2^(-p4) : 0)) then
      printks "sliding pvsanal lost a quiet sample during eviction\n", 0
      exitnowk -1
    endif
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008 26
i 1 .01 .008 56
</CsScore>
</CsoundSynthesizer>
