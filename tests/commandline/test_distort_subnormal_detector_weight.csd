<CsTest>
description = "distort retains small weighted updates"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --preserve-subnormals
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1

giShape ftgen 1,0,9,-2,1,1,1,1,1,1,1,1,1
instr 1
  kSample init 0
  kFirst init 0
  aIn = (kSample == 0 ? 1e21 : 6e20)
  aOut distort aIn, 1, giShape, 2.2e-41
  kOut downsamp aOut
  if kSample == 1 then
    kFirst = kOut
  elseif kSample == 101 then
    if !(abs(kOut/kFirst - sqrt(37)) < .00001) then
      printks "distort lost small detector updates\n", 0
      exitnowk -1
    endif
    turnoff
  endif
  kSample += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .003
</CsScore>
</CsoundSynthesizer>
