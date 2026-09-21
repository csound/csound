<CsTest>
description = "moog releases both score notes and indefinite notes"

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
gkChecks init 0

instr 1, 2
  aSig moog .5, 440, .85, 0, 0, 0, 1, 2, 2
  kRms rms aSig, 100
  kTime timeinsts
  kSteady init 0
  kTail init 0
  kDone init 0
  if kTime > .7 && kTime < .75 then
    kSteady = max(kSteady, kRms)
  endif
  if kTime > .85 && kTime < .9 then
    kTail = max(kTail, kRms)
  endif
  if kTime > 1 && kDone == 0 then
    if !(kSteady > .001 && kTail > .01*kSteady && kRms < .001*kSteady) then
      printks "moog release: steady=%g tail=%g late=%g\n", 0, kSteady, kTail, kRms
      exitnowk(-1)
    endif
    gkChecks += 1
    kDone = 1
  endif
endin

instr 3
  if gkChecks != 2 then
    printks "moog did not keep both notes alive for their release\n", 0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 256 -7 0 256 0
f 2 0 256 10 1
i 1 0 .8
i 2 0 -1
i -2 .8 0
i 3 1.2 .01
</CsScore>
</CsoundSynthesizer>
