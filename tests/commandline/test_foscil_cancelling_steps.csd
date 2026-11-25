<CsTest>
description = "foscil retains a step through cancellation"

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

giShape ftgen 1,0,-96,-2,0
instr 1
  tableiw 11, 11, giShape
  tableiw 12, 12, giShape
  kSample init 0
  kSteps[] fillarray -2^(-53), 2^(-27), -2^(-27), 0, 0, 0, 0, 0
  aCar = kSteps[min(kSample,7)]
  aOut foscil 1, sr, aCar, 0, 0, giShape, .125
  kOut downsamp aOut
  if kSample >= 3 then
    if !(kOut == 11) then
      printks "foscil read the wrong cell after cancellation\n", 0
      exitnowk -1
    endif
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008
</CsScore>
</CsoundSynthesizer>
