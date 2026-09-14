<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gkChecks init 0

instr 1
  kTriggers[] fillarray 0,1,1,1,0,1,1,1,1,1,1,1,1,1,0,1,1,0,1,1
  kX[] fillarray 0,0,0,0,0,0,.5,3,3,0,0,-2,0,1,-2,-2,-2,-2,-2,0
  kY[] fillarray 0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0
  kLimits[] fillarray -1,4,8,8,2,2,2,8,4,0,4.9,8,8,8,4,4,4,0,0,1
  ; Interior points return the limit; escape counts retain their zero-based convention.
  kExpected[] fillarray -1,4,8,8,8,2,2,0,0,0,4,8,1,2,2,4,4,4,0,1
  kChanges[] fillarray 0,1,1,0,0,1,0,1,0,0,1,1,1,1,0,1,0,0,1,1
  kCycle init 0
  kCount, kChanged mandel kTriggers[kCycle], kX[kCycle], kY[kCycle], kLimits[kCycle]
  if kCount != kExpected[kCycle] || kChanged != kChanges[kCycle] then
    printks "mandel cycle %g: count %g change %g, expected %g %g\n", 0, kCycle, kCount, kChanged, kExpected[kCycle], kChanges[kCycle]
    exitnowk -1
  endif
  kCycle += 1
  gkChecks += 1
endin

instr 2
  ; The first trigger must run even at the former coordinate sentinel.
  kCycle init 0
  kCount, kChanged mandel 1, -99999, -99999, 4
  kExpectedChange = (kCycle == 0 ? 1 : 0)
  if kCount != 0 || kChanged != kExpectedChange then
    printks "mandel skipped its first trigger at the former sentinel\n", 0
    exitnowk -1
  endif
  kCycle += 1
  gkChecks += 1
endin

instr 99
  if i(gkChecks) != 44 then
    prints "mandel cases did not all run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .3125
; Repeat in a reused instance to check initialization.
i 1 .5 .3125
i 2 .875 .0625
i 99 1 0
e
</CsScore>
</CsoundSynthesizer>
