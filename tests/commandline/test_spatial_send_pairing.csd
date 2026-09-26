<CsTest>
description = "spatial sends keep their own sources across nested init and reinit"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 2
0dbfs = 1
gkChecks init 0

opcode NestedSources, 0, 0
  aSignal = .75
  aD1, aD2 locsig aSignal, 0, 1, 1
  aS1, aS2, aS3, aS4 space aSignal, 0, 0, 1, 0, 0
  aL1, aL2 locsend
  aR1, aR2, aR3, aR4 spsend
  kL downsamp aL1
  kR downsamp aR1
  if abs(kL-.75) > .000001 || abs(kR-.75) > .000001 then
    exitnowk -1
  endif
endop

instr Child
  NestedSources
  aSignal = .5
  aD1, aD2 locsig aSignal, 0, 1, 1
  aS1, aS2, aS3, aS4 space aSignal, 0, 0, 1, 0, 0
  out aSignal
endin

instr Parent
  aSignal = p4
  aD1, aD2 locsig aSignal, 0, 1, 1
  aS1, aS2, aS3, aS4 space aSignal, 0, 0, 1, 0, 0
  NestedSources
  aChild subinstr "Child"
  kCycle init 0
  if kCycle == 20 then
    reinit SENDS
  endif
SENDS:
  aL1, aL2 locsend
  aR1, aR2, aR3, aR4 spsend
  rireturn
  kL1 downsamp aL1
  kL2 downsamp aL2
  kR1 downsamp aR1
  kR2 downsamp aR2
  kR3 downsamp aR3
  kR4 downsamp aR4
  if abs(kL1-p4)+abs(kL2-p4)+abs(kR1-p4)+abs(kR2-p4)+abs(kR3-p4)+abs(kR4-p4) > .00001 then
    prints "spatial sends used another source\n"
    exitnowk -1
  endif
  if kCycle == 21 then
    gkChecks += 1
  endif
  kCycle += 1
endin

instr MultipleSources
  aFirst = .25
  aSecond = .5
  aD1, aD2 locsig aFirst, 0, 1, 1
  aL1, aL2 locsend
  aS1, aS2, aS3, aS4 space aFirst, 0, 0, 1, 0, 0
  aR1, aR2, aR3, aR4 spsend
  aE1, aE2 locsig aSecond, 0, 1, 1
  aM1, aM2 locsend
  aT1, aT2, aT3, aT4 space aSecond, 0, 0, 1, 0, 0
  aV1, aV2, aV3, aV4 spsend
  kL downsamp aL1
  kM downsamp aM1
  kR downsamp aR1
  kV downsamp aV1
  if abs(kL-.25)+abs(kR-.25)+abs(kM-.5)+abs(kV-.5) > .00001 then
    exitnowk -1
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr Check
  if i(gkChecks) != 4 then
    prints "spatial pairing checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Parent" 0 .1 .125
i "Parent" .01 .1 .375
i "Child" .02 .1
i "MultipleSources" .12 .02
; Reuse a parent after its first note and children have ended.
i "Parent" .15 .1 .25
i "Check" .3 .01
e
</CsScore>
</CsoundSynthesizer>
