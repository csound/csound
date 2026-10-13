<CsTest>
description = "boolean struct members select branches at their declared rate"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
0dbfs = 1
#include "../libassert.orc"

opcode assertEquals(kValue:k, kExpected:k):void
 if kValue != kExpected then
  printks "expected %g, got %g\n", 0, kExpected, kValue
  exitnowk(-1)
 endif
endop

struct Flags enabled:b, sounding:B
struct Box flags:Flags

instr 1
 box:Box init
 box.flags.enabled = (p4 == 1)
 iSelected = (box.flags.enabled ? 10 : 20)
 assertEquals(iSelected, (p4 == 1 ? 10 : 20))
 SSelected = (box.flags.enabled ? "yes" : "no")
 assertEquals(strcmp(SSelected, (p4 == 1 ? "yes" : "no")), 0)
 if box.flags.enabled then
  assertEquals(p4, 1)
 else
  assertEquals(p4, 0)
 endif

 kCycle = timeinstk()
 box.flags.sounding = (kCycle % 2 == 0)
 kSelected = (box.flags.sounding ? 30 : 40)
 assertEquals(kSelected, (kCycle % 2 == 0 ? 30 : 40))
 if box.flags.sounding then
  assertEquals(kCycle % 2, 0)
 else
  assertEquals(kCycle % 2, 1)
 endif
 kLoops = 0
 while box.flags.sounding do
  kLoops += 1
  box.flags.sounding = (kLoops < 0)
 od
 assertEquals(kLoops, (kCycle % 2 == 0 ? 1 : 0))
endin

instr 2
 flags:Flags[] init 2
 flags[0].enabled = (1 == 1)
 flags[1].enabled = (1 == 0)
 assertEquals((flags[0].enabled ? 10 : 20), 10)
 assertEquals((flags[1].enabled ? 10 : 20), 20)
 kCycle = timeinstk()
 kZero = 0
 kOne = 1
 flags[kZero].sounding = (kCycle % 2 == 0)
 flags[kOne].sounding = (kCycle % 2 != 0)
 kIndex = kCycle % 2
 kSelected = (flags[kIndex].sounding ? 10 : 20)
 assertEquals(kSelected, 10)
 if flags[kIndex].sounding then
  assertEquals(kSelected, 10)
 else
  exitnowk(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 .02 .01 1
i 2 .04 .01
e
</CsScore>
</CsoundSynthesizer>
