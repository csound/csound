<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iDuration = p4 / kr
  iTicks = max(1, round(p4))
  kInput init p5
  kPerfInput = p5
  kCycle init 0
  kCycle += 1
  if kCycle == 6 then
    reinit HOLD
  endif
HOLD:
  kFromInit trighold kInput, iDuration
  kFromPerf trighold kPerfInput, iDuration
  kAlias sctrig kInput, iDuration
  ; Preserve the output available to init-time consumers.
  if i(kFromInit) != p5 || i(kAlias) != p5 then
    prints "trighold changed its initial output\n"
    exitnow(-1)
  endif
  rireturn
  kAge = (kCycle - 1) % 5
  kExpected = kAge < iTicks ? p5 : 0
  if kFromInit != kExpected || kFromPerf != kExpected || kAlias != kExpected then
    printks "trighold duration %g cycles: wrong output on cycle %d\n", 0, p4, kCycle
    exitnowk(-1)
  endif
  if kCycle == 10 then
    gkChecks += 1
  endif
endin

instr 2
  ; Compare both rates at one sample per control cycle.
  setksmps 1
  kInput init .25
  aInput init .25
  kHold trighold kInput, 3 / sr
  aHold trighold aInput, 3 / sr
  kAudio downsamp aHold
  kCycle init 0
  kCycle += 1
  kExpected = kCycle <= 3 ? .25 : 0
  if kHold != kExpected || kAudio != kExpected then
    printks "trighold audio/control duration differs on sample %d\n", 0, kCycle
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 5 then
    prints "trighold checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 3 .25
i 1 0 .01 1 1
i 1 0 .01 .4 .5
i 1 0 .01 0 .75
i 2 .02 .001
i 99 .03 .001
</CsScore>
</CsoundSynthesizer>
