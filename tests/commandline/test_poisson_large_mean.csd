<CsTest>
description = "Poisson sampling handles large means at all rates and in GEN21"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
seed 12345
instr 2
  iZero poisson 0
  iInvalid poisson exp(1000)
  kZero poisson 0
  if iZero != 0 || iInvalid != 0 then
    exitnow -1
  endif
  if kZero != 0 then
    exitnowk -1
  endif
endin
instr 1
  iSample poisson p4
  iTable ftgen 0, 0, -16, -21, 11, p4
  iIndex = 0
  while iIndex < 16 do
    iValue table iIndex, iTable
    if iValue < 0 || iValue != int(iValue) || abs(iValue - p4) > 10 * sqrt(p4) then
      exitnow -1
    endif
    iIndex += 1
  od
  if iSample < 0 || abs(iSample - p4) > 10 * sqrt(p4) then
    exitnow -1
  endif
  kSample poisson p4
  aSample poisson p4
  kAudio downsamp aSample
  if kSample < 0 || abs(kSample - p4) > 10 * sqrt(p4) || kAudio < 0 || abs(kAudio - p4) > 10 * sqrt(p4) then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0625 104
i 1 .0625 .0625 1000
i 1 .125 .0625 1000000
i 2 .1875 .0625
e
</CsScore>
</CsoundSynthesizer>
