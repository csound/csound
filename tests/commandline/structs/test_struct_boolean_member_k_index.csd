<CsTest>
description = "a b member of a struct array's element read at a k index follows the index"

[expect]
exit = 0
output = ["every b member read at a k index checked"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
nchnls = 1
0dbfs = 1

struct Flag on:b

opcode CheckInit(iValue:i, iExpected:i, SWhat:S):void
 if iValue != iExpected then
  prints("%s: expected %g, got %g\n", SWhat, iExpected, iValue)
  exitnow(-1)
 endif
endop

opcode Check(kValue:k, kExpected:k, SWhat:S):void
 if kValue != kExpected then
  printks("%s: expected %g, got %g\n", 0, SWhat, kExpected, kValue)
  exitnowk(-1)
 endif
endop

; fs[kIndex].on is true in odd cycles, as kIndex is 1 there
instr 1
 kc = timeinstk()
 kIndex = kc % 2
 fs:Flag[] init 2
 fs[0].on = 1 == 2
 fs[1].on = 1 == 1

 Check(fs[kIndex].on ? 1 : 0, kIndex, "as a ternary's condition")
 value:B = fs[kIndex].on
 Check(value ? 1 : 0, kIndex, "into a B")
 both:B = fs[kIndex].on && true
 Check(both ? 1 : 0, kIndex, "in &&")

 ; an index that is itself read at a k index changes too
 iSlots:i[] fillarray 0, 1
 nested:B = fs[iSlots[kIndex]].on
 Check(nested ? 1 : 0, kIndex, "at an index read from an i[] at a k index")

 kThen init 0
 kWant init 0
 if fs[kIndex].on then
  kThen += 1
 endif
 kWant += kIndex
 Check(kThen, kWant, "as an if's condition")

 ; writing at a k index still writes the member
 gs:Flag[] init 2
 gs[kIndex].on = true
 Check(gs[kIndex].on ? 1 : 0, 1, "written at a k index")
 Check(gs[1 - kIndex].on ? 1 : 0, kc > 1 ? 1 : 0, "the other member")

 ; at an i index, the member is read at init
 CheckInit(fs[1].on ? 1 : 0, 1, "at an i index")

 if kc == 4 then
  printks("every b member read at a k index checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
