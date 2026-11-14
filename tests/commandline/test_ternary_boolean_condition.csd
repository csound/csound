<CsTest>
description = "a ternary takes a name, true or false, and a boolean ternary is a condition"

[expect]
exit = 0
output = ["every ternary checked"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 4
nchnls = 1
0dbfs = 1

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


opcode Pick(v:B):k
 xout v ? 5 : 6
endop

opcode PickInit(v:b):i
 xout v ? 5 : 6
endop

; Global code selects at init, on a b or a B
yes@global:b = 1 < 2
on@global:B init 1
gName@global:i = yes ? 1 : 2
gFalse@global:i = false ? 1 : 2
gNameB@global:i = on ? 1 : 2
gFalsek@global:i = falsek ? 1 : 2

; On a b, true or false, the ternary selects at init
instr 1
 v:b = p4 == 1
 CheckInit(v ? 10 : 20, p4 == 1 ? 10 : 20, "a b")
 CheckInit(true ? 1 : 2, 1, "true")
 CheckInit(false ? 1 : 2, 2, "false")
 SPick = v ? "yes" : "no"
 CheckInit(strcmp(SPick, p4 == 1 ? "yes" : "no"), 0, "a b, string")
 CheckInit(PickInit(p4 == 1), p4 == 1 ? 5 : 6, "a b parameter")
 CheckInit((p4 == 1 ? true : false) ? 10 : 20, p4 == 1 ? 10 : 20,
           "a b ternary as the condition")
 iThen, iElse init 0
 if (p4 == 1 ? true : false) then
  iThen += 1
 else
  iElse += 1
 endif
 CheckInit(iThen, p4, "if on a b ternary, then")
 CheckInit(iElse, 1 - p4, "if on a b ternary, else")
 CheckInit(gName, 1, "a b in global code")
 CheckInit(gFalse, 2, "false in global code")
 CheckInit(gNameB, 1, "a B in global code")
 CheckInit(gFalsek, 2, "falsek in global code")
endin

; On a B, truek or falsek, it selects each cycle; v is true on even ones
instr 2
 kCycle = timeinstk()
 kEven = kCycle % 2 == 0 ? 1 : 0
 v:B = kCycle % 2 == 0
 Check(v ? 10 : 20, 20 - 10 * kEven, "a B")
 Check(truek ? 1 : 2, 1, "truek")
 Check(falsek ? 1 : 2, 2, "falsek")
 SPick = v ? "even" : "odd"
 Check(strcmpk(SPick, kEven == 1 ? "even" : "odd"), 0, "a B, string")
 aPick = v ? a(1) : a(2)
 Check(vaget(3, aPick), 2 - kEven, "a B, audio")
 Check(Pick(kCycle % 2 == 0), 6 - kEven, "a B parameter")
endin

instr 3
 prints("every ternary checked\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 0
i 1 0.02 0.01 1
i 2 0.04 0.02
i 3 0.08 0.01
e
</CsScore>
</CsoundSynthesizer>
