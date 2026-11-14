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

; A loop stops itself if it runs at the wrong rate, so that a wrong rate
; fails here rather than spinning.
opcode Limit(iPass:i, iLimit:i, SWhat:S):void
 if iPass > iLimit then
  prints("%s: ran %g times at init\n", SWhat, iPass)
  exitnow(-1)
 endif
endop

opcode IsEven(kValue:k):B
 xout kValue % 2 == 0
endop

opcode IsOne(iValue:i):b
 xout iValue == 1
endop

opcode Not(v:b):b
 xout !v
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

; An element of a boolean array, and a call with a computed argument
instr 5
 kCycle = timeinstk()
 bk:B[] init 2
 kOne = 1
 bk[kOne] = kCycle % 2 == 0
 bi:b[] fillarray false, p4 == 0
 kElement, kCall, kCallElse init 0
 iElement, iCall init 0

 if bk[kOne] then
  kElement += 1
 endif
 if bi[1] then
  iElement = 1
 endif
 if IsEven(kCycle + 1) then
  kCall += 1
 else
  kCallElse += 1
 endif
 if IsOne(p4 + 1) then
  iCall = 1
 endif
 if Not(p4 == 0) then
  iCall += 10
 endif

 Check(kElement, int(kCycle / 2), "an element of a B[]")
 CheckInit(iElement, 1 - p4, "an element of a b[]")
 Check(kCall, kCycle - int(kCycle / 2), "a call with a computed argument")
 Check(kCallElse, int(kCycle / 2), "a call with a computed argument, else")
 CheckInit(iCall, 1 + 9 * p4, "an i-rate call with a computed argument")

 ; An element of a b[] read at a k index changes at performance
 bs:b[] fillarray false, true
 kIndex = kCycle > 1 ? 1 : 0
 iWhile init 0
 kWhile = 0
 while bs[kIndex] do
  iWhile += 1
  Limit(iWhile, 1, "while on a b[] element at a k index")
  kWhile += 1
  if kWhile == 3 then
   break
  endif
 od
 Check(kWhile, kCycle > 1 ? 3 : 0, "while on a b[] element at a k index")
 Check(bs[kIndex] ? 10 : 20, kCycle > 1 ? 10 : 20,
       "a ternary on a b[] element at a k index")
 ; it has no value at init, so both branches get their init pass, as on a B
 iBranches init 0
 kRamp, kThenIndex, kElseIndex init 0
 if bs[kIndex] then
  iBranches += 1
  kRamp = line(0, 1, 1000)
  kThenIndex += 1
 else
  iBranches += 1
  kElseIndex += 1
 endif
 CheckInit(iBranches, 2, "if on a b[] element at a k index, init")
 Check(kThenIndex, kCycle - 1, "if on a b[] element at a k index, then")
 Check(kElseIndex, 1, "if on a b[] element at a k index, else")
 Check(kRamp > 0 ? 1 : 0, kCycle > 2 ? 1 : 0,
       "if on a b[] element at a k index, an opcode in the then branch")

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
