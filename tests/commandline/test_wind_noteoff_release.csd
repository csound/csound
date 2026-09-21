<CsTest>
[expect]
exit = 0
stderr = ["wind release 1 passed", "wind release 2 passed"]
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
; A quarter-second release must also run for indefinite notes.
instr 1, 2
  if p1 == 1 then
    aSig wgclar .5, 440, -.3, .01, .25, 0, 0, 0
  else
    aSig wgflute .5, 440, .32, .01, .25, 0, 0, 0
  endif
  kTime timeinsts
  kRms rms aSig
  kSteady init 0
  if kTime > .2 && kTime < .24 then
    kSteady = max(kSteady,kRms)
  endif
  kDone init 0
  if kTime > .47 && kDone == 0 then
    if !(kSteady > .01 && kRms < .3*kSteady) then
      printks "wind note-off did not release the envelope\n", 0
      exitnowk(-1)
    endif
    printks "wind release %g passed\n", 0, p1
    kDone = 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 -1
i 2 0 -1
i -1 .25 0
i -2 .25 0
f 0 .6
</CsScore>
</CsoundSynthesizer>
