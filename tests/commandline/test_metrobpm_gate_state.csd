<CsTest>
description = "metrobpm keeps its gate state and wraps whole cycles"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 128
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kCycle init 0
  kExpected[] fillarray 0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1
  ; The gate must neither start from nor hold the output variable's value.
  kGate = 7
  kGate metrobpm 60, .125, .5
  kPulse metrobpm 60
  if kGate != kExpected[kCycle] || kPulse != (kCycle % 8 == 0 ? 1 : 0) then
    printks "metrobpm gate mismatch at cycle %g\n", 0, kCycle
    exitnowk(-1)
  endif
  if kCycle == 15 then
    gkChecks += 1
  endif
  kCycle += 1
endin

instr 2
  kCycle init 0
  ; Three whole cycles in one control period must leave no queued ticks.
  kBpm = (kCycle == 0 ? 1440 : 0)
  kPulse metrobpm kBpm, .125
  if kPulse != (kCycle == 0 ? 1 : 0) then
    printks "metrobpm emitted a queued tick at cycle %g\n", 0, kCycle
    exitnowk(-1)
  endif
  if kCycle == 3 then
    gkChecks += 1
  endif
  kCycle += 1
endin

instr 99
  if i(gkChecks) != 2 then
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 2
i 2 0 .5
i 99 2 .125
</CsScore>
</CsoundSynthesizer>
