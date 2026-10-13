<CsTest>
description = "GEN49 sets playback rate and channel metadata for loscil"

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
instr 1
  iTable ftgen 0, 0, p4, -49, "beats.mp3", 0, p5
  if ftchnls(iTable) != p5 then
    prints "GEN49 set the wrong channel count\n"
    exitnow(-1)
  endif
  if p5 == 1 then
    aPhase, aSound loscilphs 1, 1, iTable, 1, 0
  else
    aPhase, aLeft, aRight loscilphs 1, 1, iTable, 1, 0
  endif
  kFrame init 0
  kPhase downsamp aPhase
  kExpected = kFrame * (44100/sr) / ftlen(iTable)
  if !(abs(kPhase-kExpected) < .000001) then
    printks "GEN49 playback phase: %g, expected %g\n", 0, kPhase, kExpected
    exitnowk(-1)
  endif
  kFrame += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .01 8192 1
i 1 0 .01 8192 2
i 1 0 .01 0 1
i 1 0 .01 0 2
</CsScore>
</CsoundSynthesizer>
