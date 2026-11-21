<CsTest>
description = "while and until take a named boolean's rate from its type, not its spelling"

[expect]
exit = 0
output = ["every loop checked"]
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

; Each loop stops itself if it runs at the wrong rate, so that a wrong rate
; fails here rather than spinning.
opcode Limit(iPass:i, iLimit:i, SWhat:S):void
 if iPass > iLimit then
  prints("%s: ran %g times at init\n", SWhat, iPass)
  exitnow(-1)
 endif
endop

opcode LimitK(kPass:k, SWhat:S):void
 if kPass > 8 then
  printks("%s: ran %g times in one cycle\n", 0, SWhat, kPass)
  exitnowk(-1)
 endif
endop

flag@global:B init 0

; Global code has no performance: a loop there runs on init jumps, on any type
globalPasses@global:i = 0
globalDone@global:B init 0
until globalDone do
 globalPasses += 1
 Limit(globalPasses, 3, "until in global code")
 globalDone = globalPasses >= 3
od

spelledPasses@global:i = 0
gBdone init 0
until gBdone do
 spelledPasses += 1
 Limit(spelledPasses, 3, "until on a gB in global code")
 gBdone = spelledPasses >= 3
od

; A B is k-rate: the body has one init pass, and loops each cycle
instr 1
 iWhile, iUntil, iOld, iGlobal, iBreak, iContinue = 0, 0, 0, 0, 0, 0

 kCount = 0
 v:B = kCount < 3
 while v do
  iWhile += 1
  Limit(iWhile, 1, "while on a :B")
  kCount += 1
  LimitK(kCount, "while on a :B")
  v = kCount < 3
 od
 Check(kCount, 3, "while on a :B")

 kCount = 0
 w:B = kCount >= 3
 until w do
  iUntil += 1
  Limit(iUntil, 1, "until on a :B")
  kCount += 1
  LimitK(kCount, "until on a :B")
  w = kCount >= 3
 od
 Check(kCount, 3, "until on a :B")

 kCount = 0
 Bflag = kCount < 3
 while Bflag do
  iOld += 1
  Limit(iOld, 1, "while on an old-style B")
  kCount += 1
  LimitK(kCount, "while on an old-style B")
  Bflag = kCount < 3
 od
 Check(kCount, 3, "while on an old-style B")

 kCount = 0
 flag = kCount >= 3
 until flag do
  iGlobal += 1
  Limit(iGlobal, 1, "until on a global B")
  kCount += 1
  LimitK(kCount, "until on a global B")
  flag = kCount >= 3
 od
 Check(kCount, 3, "until on a global B")

 kCount = 0
 x:B = kCount < 3
 while x do
  iBreak += 1
  Limit(iBreak, 1, "break from a :B")
  kCount += 1
  LimitK(kCount, "break from a :B")
  if kCount == 2 then
   break
  endif
  x = kCount < 3
 od
 Check(kCount, 2, "break from a :B")

 kCount = 0
 kAfter = 0
 y:B = kCount < 3
 while y do
  iContinue += 1
  Limit(iContinue, 1, "continue in a :B")
  kCount += 1
  LimitK(kCount, "continue in a :B")
  y = kCount < 3
  if kCount == 2 then
   continue
  endif
  kAfter += 1
 od
 Check(kCount, 3, "continue in a :B")
 Check(kAfter, 2, "continue in a :B, after it")

 CheckInit(globalPasses, 3, "until in global code")
 CheckInit(spelledPasses, 3, "until on a gB in global code")
endin

; A b is i-rate, whatever its name holds: the loop runs at init, and not after
instr 2
 iCount, iSpelt, iOld = 0, 0, 0
 kPerf init 0

 v:b = iCount < 3
 while v do
  iCount += 1
  Limit(iCount, 3, "while on a b")
  v = iCount < 3
 od
 CheckInit(iCount, 3, "while on a b")

 xB:b = iSpelt < 3
 while xB do
  iSpelt += 1
  Limit(iSpelt, 3, "while on a b named xB")
  kPerf += 1
  LimitK(kPerf, "while on a b named xB")
  xB = iSpelt < 3
 od
 CheckInit(iSpelt, 3, "while on a b named xB")
 Check(kPerf, 0, "while on a b named xB, in performance")

 bB = iOld >= 3
 until bB do
  iOld += 1
  Limit(iOld, 3, "until on a b named bB")
  kPerf += 1
  LimitK(kPerf, "until on a b named bB")
  bB = iOld >= 3
 od
 CheckInit(iOld, 3, "until on a b named bB")
 Check(kPerf, 0, "until on a b named bB, in performance")
endin

instr 3
 prints("every loop checked\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.02 0.01
i 3 0.04 0.01
e
</CsScore>
</CsoundSynthesizer>
