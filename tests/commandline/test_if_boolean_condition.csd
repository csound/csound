<CsTest>
description = "a condition can be a name, true or false, an array element or a call"

[expect]
exit = 0
output = ["every condition checked"]
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

; Global code takes init jumps on a name, true or false
yes@global:b = 1 < 2
gGoto@global:i = 0
if yes goto globalYes
gGoto += 1
globalYes:
if false goto globalFalse
gGoto += 10
globalFalse:
gElseif@global:i = 0
if false then
 gElseif = 1
elseif yes then
 gElseif = 2
elseif true then
 gElseif = 3
endif

; if v goto: goto jumps at init and performance, igoto at init, kgoto at
; performance. The statements after each jump count the times it fell through.
instr 1
 v:b = p4 == 1
 iGoto, iIgoto, iTrue, iFalse init 0
 kGoto, kIgoto, kKgoto, kTrue, kFalse init 0

 if v goto skipGoto
 iGoto += 1
 kGoto += 1
skipGoto:
 if v igoto skipIgoto
 iIgoto += 1
skipIgoto:
 kIgoto += 1
 if v kgoto skipKgoto
 kKgoto += 1
skipKgoto:
 if true goto skipTrue
 iTrue += 1
 kTrue += 1
skipTrue:
 if false goto skipFalse
 iFalse += 1
 kFalse += 1
skipFalse:

 CheckInit(iGoto, 1 - p4, "goto on a b, init")
 Check(kGoto, (1 - p4) * timeinstk(), "goto on a b")
 CheckInit(iIgoto, 1 - p4, "igoto on a b")
 Check(kIgoto, timeinstk(), "igoto on a b, performance")
 Check(kKgoto, (1 - p4) * timeinstk(), "kgoto on a b")
 CheckInit(iTrue, 0, "goto on true, init")
 Check(kTrue, 0, "goto on true")
 CheckInit(iFalse, 1, "goto on false, init")
 Check(kFalse, timeinstk(), "goto on false")
endin

; On a B, kgoto and goto read it each cycle, which here is true on even ones
instr 2
 kCycle = timeinstk()
 v:B = kCycle % 2 == 0
 kKgoto, kGoto, kTruek, kFalsek init 0
 if v kgoto skipKgoto
 kKgoto += 1
skipKgoto:
 if v goto skipGoto
 kGoto += 1
skipGoto:
 if truek kgoto skipTruek
 kTruek += 1
skipTruek:
 if falsek kgoto skipFalsek
 kFalsek += 1
skipFalsek:
 Check(kKgoto, kCycle - int(kCycle / 2), "kgoto on a B")
 Check(kGoto, kCycle - int(kCycle / 2), "goto on a B")
 Check(kTruek, 0, "kgoto on truek")
 Check(kFalsek, kCycle, "kgoto on falsek")
endin

; elseif on a b, true or false, mixed with comparisons: one branch runs
instr 3
 v:b = p4 == 1
 w:b = p4 == 2
 flag:b = 0
 iBranch init 0
 kBranch init 0
 if p4 == 0 then
  iBranch = 1
  kBranch += 1
 elseif v then
  iBranch = 2
  kBranch += 2
 elseif false then
  iBranch = 9
 elseif w then
  iBranch = 3
  kBranch += 3
 elseif true then
  iBranch = 4
  kBranch += 4
 else
  iBranch = 8
 endif
 CheckInit(iBranch, p4 + 1, "elseif on a b")
 Check(kBranch, (p4 + 1) * timeinstk(), "elseif on a b, performance")

 ; issue 3619's case: neither branch
 iNeither init 0
 if false then
  iNeither += 1
 elseif flag then
  iNeither += 1
 endif
 CheckInit(iNeither, 0, "elseif on a false b")
endin

; elseif on a B, mixed with comparisons: each branch gets its init pass, and
; each cycle takes the one its condition picks
instr 4
 kCycle = timeinstk()
 v:B = kCycle % 3 == 1
 w:B = kCycle % 3 == 2
 iInits init 0
 kFirst, kSecond, kThird, kNever init 0
 if kCycle % 3 == 0 then
  iInits += 1
  kFirst += 1
 elseif v then
  iInits += 1
  kSecond += 1
 elseif falsek then
  iInits += 1
  kNever += 1
 elseif w then
  iInits += 1
  kThird += 1
 endif
 CheckInit(iInits, 4, "elseif on a B, init")
 Check(kFirst, int(kCycle / 3), "elseif on a B, comparison")
 Check(kSecond, int((kCycle + 2) / 3), "elseif on a B")
 Check(kThird, int((kCycle + 1) / 3), "elseif on a second B")
 Check(kNever, 0, "elseif on falsek")
endin

instr 5
 CheckInit(gGoto, 10, "if-goto in global code")
 CheckInit(gElseif, 2, "elseif in global code")
endin

instr 6
 prints("every condition checked\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 0
i 1 0.02 0.01 1
i 2 0.04 0.01
i 3 0.06 0.01 0
i 3 0.08 0.01 1
i 3 0.1 0.01 2
i 3 0.12 0.01 3
i 4 0.14 0.01
i 5 0.16 0.01 0
i 5 0.18 0.01 1
i 6 0.2 0.01
e
</CsScore>
</CsoundSynthesizer>
