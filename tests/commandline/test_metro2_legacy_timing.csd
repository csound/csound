<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1024
ksmps=64
nchnls=1
gkChecks init 0
instr 1
 ; Tick sequences from the original metro2, including startup and endpoints.
 kExpected[] fillarray 1,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,0,
    -1,0,0,1,0,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,-1,0,0,0,0,
    -1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,
    -1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
 kDefault metro2 2,p4,-1,p5
 kLegacy metro2 2,p4,-1,p5,0
 kCycle timeinstk
 kReference = kExpected[p6*32+kCycle-1]
 if kDefault != kReference || kLegacy != kReference then
  printks "metro2 legacy swing=%g phase=%g cycle=%g: default=%g explicit=%g expected=%g\n",0,p4,p5,kCycle,kDefault,kLegacy,kReference
  exitnowk(1)
 endif
 if kCycle == 32 then
  gkChecks += 1
  turnoff
 endif
endin
instr 99
 if i(gkChecks) != 4 then
  prints "metro2 legacy checks did not complete\n"
  exitnow(1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 2 .5 0 0
i 1 0 2 .5 .75 1
i 1 0 2 0 0 2
i 1 0 2 1 0 3
i 99 2.1 .1
e
</CsScore>
</CsoundSynthesizer>
