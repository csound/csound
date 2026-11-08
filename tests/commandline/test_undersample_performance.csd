<CsTest>
description = "undersample runs once per caller cycle and preserves control signals"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1

gkCalls init 0
gkDoubleTime init 0

opcode HalfRate, iikk, k
  undersample 2, 4
  kInput xin
  gkCalls += 1
  kOutput = kInput + 7
  kTime times
  xout sr, ksmps, kOutput, kTime
endop

opcode QuarterRate(kInput:k):(i,i,k,k)
  undersample 4, 4
  gkCalls += 1
  kOutput = kInput + 7
  kTime times
  xout sr, ksmps, kOutput, kTime
endop

opcode NestedHalfRate, k, k
  undersample 2, 4
  kInput xin
  iInnerSr, iInnerBlock, kOutput, kTime HalfRate kInput
  if iInnerSr != 8 || iInnerBlock != 2 then
    exitnow -1
  endif
  xout kOutput
endop

opcode DoubleRate, 0, 0
  oversample 2, 4
  ; Use a global result to observe time without filtering it through xout.
  gkDoubleTime times
endop

instr Check
  gkCalls = 0
  kInput = timeinstk()
  iHalfSr, iHalfBlock, kHalf, kHalfTime HalfRate kInput
  iQuarterSr, iQuarterBlock, kQuarter, kQuarterTime QuarterRate kInput
  kNested NestedHalfRate kInput
  DoubleRate
  if iHalfSr != 16 || iHalfBlock != 4 || \
     iQuarterSr != 8 || iQuarterBlock != 2 then
    prints "undersample set the wrong local sample rate or block size\n"
    exitnow -1
  endif
  ; Smaller blocks cover the same time, so each counted body must run once.
  ; Control inputs and outputs must arrive in this cycle, without filtering.
  if gkCalls != 3 || kHalf != kInput + 7 || kQuarter != kInput + 7 || \
     kNested != kInput + 7 then
    printks "body calls=%g, input=%g, half=%g, quarter=%g, nested=%g\n", \
            0, gkCalls, kInput, kHalf, kQuarter, kNested
    exitnowk -1
  endif
  ; Starting a note later must not scale the inherited control-cycle count.
  kParentTime times
  if kHalfTime != kParentTime || kQuarterTime != kParentTime || \
     gkDoubleTime != kParentTime then
    printks "parent time=%g, half=%g, quarter=%g, double=%g\n", \
            0, kParentTime, kHalfTime, kQuarterTime, gkDoubleTime
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i "Check" 0 0.5
i "Check" 1 0.5
e
</CsScore>
</CsoundSynthesizer>
