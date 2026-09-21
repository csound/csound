<CsTest>
[expect]
exit = 0
stderr = ["wgbow release passed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
 xtratim .2
 aSig wgbow .5, 440, 3, .127, 0, 0
 kTime timeinsts
 kRms rms aSig
 kSteady init 0
 if kTime > .2 && kTime < .24 then
  kSteady = max(kSteady, kRms)
 endif
 kDone init 0
 if kTime > .4 && kDone == 0 then
  kReleasing release
  if !(kReleasing == 1 && kSteady > .01 && kRms < .1*kSteady) then
   printks "wgbow did not release after note-off\n", 0
   exitnowk(-1)
  endif
  printks "wgbow release passed\n", 0
  kDone = 1
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 -1
i -1 .25 0
f 0 .5
</CsScore>
</CsoundSynthesizer>
