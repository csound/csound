<CsTest>
description = "scale2 bypasses disabled smoothing and keeps normal scaling"
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
instr 1
  kCycle init 0
  kNegative init -1
  ; A transient NaN must not remain in the output when smoothing is off.
  kInput = (kCycle == 0 ? log(kNegative) : .5)
  kDirect scale2 kInput, 0, 1
  kSmooth scale2 .5, 0, 1, 0, 1, 1/kr
  kReverse scale2 .25, 1, 0
  kLow scale2 -1, 0, 1
  kHigh scale2 2, 0, 1
  if kCycle > 0 && kDirect != .5 then
    printks "scale2 retained stale history with smoothing disabled\n", 0
    exitnowk(-1)
  endif
  if !(abs(kSmooth - .5*(1-pow(.5,kCycle+1))) < .00001) || kReverse != .75 || kLow != 0 || kHigh != 1 then
    printks "scale2 scaling or smoothing mismatch\n", 0
    exitnowk(-1)
  endif
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .5
</CsScore>
</CsoundSynthesizer>
