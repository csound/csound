<CsTest>
description = "voice accepts an amplitude assigned at k-rate"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  kAmp = .8
  aVoice voice kAmp, 200, 1, .488, 0, 0, 1, 2
  kPeak peak aVoice
  kTime timeinsts
  if kTime > .2 && !(kPeak > .0001) then
    printks "voice ignored its k-rate amplitude\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 256 10 1
f 2 0 256 10 1
i 1 0 .25
</CsScore>
</CsoundSynthesizer>
