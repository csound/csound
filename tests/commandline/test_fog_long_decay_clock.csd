<CsTest>
description = "fog decay advances even below the float phase spacing"

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

giWave ftgen 1,0,-3,-2,1,1,1,1
giEnv ftgen 2,0,16777216,-2,0
instr 1
  tableiw .75, 16777215, giEnv
  tableiw .50, 16777214, giEnv
  tableiw .25, 16777213, giEnv
  aPos init 0
  aOut fog 1, 0, 0, aPos, 0, 0, 0, 32768, 32768, 4, giWave, giEnv, 32768
  kValue downsamp aOut
  kSample init 0
  if kSample == 4 && kValue != .25 then
    printks "fog decay did not advance to the third cell\n", 0
    exitnowk -1
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .008
</CsScore>
</CsoundSynthesizer>
