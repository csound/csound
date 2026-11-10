<CsTest>
description = "if takes a named boolean's rate from its type, not its spelling"

[expect]
exit = 0
output = ["every branch checked"]
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

; A B is k-rate: each cycle picks a branch, so both need their init pass, and
; an opcode in either must be ready. The first cycle is odd and takes the else.
opcode CheckCycles(kThen:k, kElse:k, SWhat:S):void
 kCycle = timeinstk()
 Check(kThen, int(kCycle / 2), SWhat)
 Check(kElse, kCycle - int(kCycle / 2), SWhat)
endop

flag@global:B init 0
gBflag init 0

; Global code has no performance, so its jumps are init jumps. Nothing reads a
; B there: what is checked is that one branch runs, however the name is spelt
globalBranches@global:i = 0
if flag then
 globalBranches += 1
else
 globalBranches += 1
endif
if gBflag then
 globalBranches += 1
else
 globalBranches += 1
endif

opcode FromArgument(v:B):void
 kThen, kElse init 0
 iThen, iElse init 0
 if v then
  iThen = 1
  aOut oscili 1, 100
  kThen += 1
 else
  iElse = 1
  aOut oscili 1, 100
  kElse += 1
 endif
 CheckInit(iThen + iElse, 2, "argument init")
 CheckCycles(kThen, kElse, "argument")
endop

instr 1
 kCycle = timeinstk()
 kThen1, kElse1, kThen2, kElse2, kThen3, kElse3, kThen4, kElse4 init 0
 iThen1, iElse1, iThen2, iElse2, iThen3, iElse3, iThen4, iElse4 init 0

 v:B = kCycle % 2 == 0
 if v then
  iThen1 = 1
  aThen1 oscili 1, 100
  kThen1 += 1
 else
  iElse1 = 1
  aElse1 oscili 1, 100
  kElse1 += 1
 endif
 CheckInit(iThen1 + iElse1, 2, ":B init")
 CheckCycles(kThen1, kElse1, ":B")

 Bflag = kCycle % 2 == 0
 if Bflag then
  iThen2 = 1
  aThen2 oscili 1, 100
  kThen2 += 1
 else
  iElse2 = 1
  aElse2 oscili 1, 100
  kElse2 += 1
 endif
 CheckInit(iThen2 + iElse2, 2, "old-style B init")
 CheckCycles(kThen2, kElse2, "old-style B")

 flag = kCycle % 2 == 0
 if flag then
  iThen3 = 1
  aThen3 oscili 1, 100
  kThen3 += 1
 else
  iElse3 = 1
  aElse3 oscili 1, 100
  kElse3 += 1
 endif
 CheckInit(iThen3 + iElse3, 2, "global B init")
 CheckCycles(kThen3, kElse3, "global B")

 gBflag = kCycle % 2 == 0
 if gBflag then
  iThen4 = 1
  aThen4 oscili 1, 100
  kThen4 += 1
 else
  iElse4 = 1
  aElse4 oscili 1, 100
  kElse4 += 1
 endif
 CheckInit(iThen4 + iElse4, 2, "old-style global B init")
 CheckCycles(kThen4, kElse4, "old-style global B")

 FromArgument(kCycle % 2 == 0)
endin

; An ithen's are too
instr 5
 iBranches init 0
 v:B = 0
 if v ithen
  iBranches += 1
 else
  iBranches += 1
 endif
 xB:B = 0
 if xB ithen
  iBranches += 1
 else
  iBranches += 1
 endif
 CheckInit(iBranches, 2, "ithen on a B")
 CheckInit(globalBranches, 2, "global code")
endin

; falsek is a B too: only its else branch performs, and that needs its init
instr 2
 kElse init 0
 if falsek then
  aThen oscili 1, 100
 else
  aElse oscili 1, 100
  kElse += 1
 endif
 Check(kElse, timeinstk(), "falsek")
endin

; A b is i-rate, whatever its name holds: one branch runs, at init and after
opcode CheckOneBranch(iThen:i, iElse:i, iExpectedThen:i, SWhat:S):void
 CheckInit(iThen, iExpectedThen, SWhat)
 CheckInit(iElse, 1 - iExpectedThen, SWhat)
endop

instr 3
 iThen1, iElse1, iThen2, iElse2, iThen3, iElse3, iThen4, iElse4 init 0
 kThen1, kElse1, kThen2, kElse2 init 0

 xB:b = p4 == 1
 if xB then
  iThen1 = 1
  kThen1 += 1
 else
  iElse1 = 1
  kElse1 += 1
 endif
 CheckOneBranch(iThen1, iElse1, p4, "b named xB")
 Check(kThen1, p4 * timeinstk(), "b named xB, then")
 Check(kElse1, (1 - p4) * timeinstk(), "b named xB, else")

 bB = p4 == 1
 if bB then
  iThen2 = 1
 else
  iElse2 = 1
 endif
 CheckOneBranch(iThen2, iElse2, p4, "old-style b named bB")

 v:b = p4 == 1
 if v then
  iThen3 = 1
  kThen2 += 1
 else
  iElse3 = 1
  kElse2 += 1
 endif
 CheckOneBranch(iThen3, iElse3, p4, "b")
 Check(kThen2, p4 * timeinstk(), "b, then")
 Check(kElse2, (1 - p4) * timeinstk(), "b, else")

 if true then
  iThen4 = 1
 else
  iElse4 = 1
 endif
 CheckOneBranch(iThen4, iElse4, 1, "true")
endin

instr 4
 prints("every branch checked\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 0.01
i 3 0.04 0.01 0
i 3 0.06 0.01 1
i 5 0.08 0.01
i 4 0.1 0.01
e
</CsScore>
</CsoundSynthesizer>
