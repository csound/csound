<CsTest>
description = "while and until take true, false, truek and falsek"

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

; Global code has no performance: its loops run on init jumps
gWhileFalse@global:i = 0
while false do
 gWhileFalse += 1
 Limit(gWhileFalse, 0, "while false in global code")
od
gUntilTrue@global:i = 0
until true do
 gUntilTrue += 1
 Limit(gUntilTrue, 0, "until true in global code")
od
gWhileTrue@global:i = 0
while true do
 gWhileTrue += 1
 Limit(gWhileTrue, 3, "while true in global code")
 if gWhileTrue == 3 then
  break
 endif
od
gUntilFalse@global:i = 0
until false do
 gUntilFalse += 1
 Limit(gUntilFalse, 3, "until false in global code")
 if gUntilFalse == 3 then
  break
 endif
od
gUntilTruek@global:i = 0
until truek do
 gUntilTruek += 1
 Limit(gUntilTruek, 0, "until truek in global code")
od
gUntilFalsek@global:i = 0
until falsek do
 gUntilFalsek += 1
 Limit(gUntilFalsek, 3, "until falsek in global code")
 if gUntilFalsek == 3 then
  break
 endif
od

; true and false are b: the loops run at init
instr 1
 iWhileFalse, iUntilTrue, iWhileTrue, iUntilFalse init 0
 kWhileFalse, kUntilTrue init 0

 while false do
  iWhileFalse += 1
  Limit(iWhileFalse, 0, "while false")
  kWhileFalse += 1
 od
 until true do
  iUntilTrue += 1
  Limit(iUntilTrue, 0, "until true")
  kUntilTrue += 1
 od
 while true do
  iWhileTrue += 1
  Limit(iWhileTrue, 3, "while true")
  if iWhileTrue == 3 then
   break
  endif
 od
 until false do
  iUntilFalse += 1
  Limit(iUntilFalse, 3, "until false")
  if iUntilFalse == 3 then
   break
  endif
 od

 CheckInit(iWhileFalse, 0, "while false")
 Check(kWhileFalse, 0, "while false, performance")
 CheckInit(iUntilTrue, 0, "until true")
 Check(kUntilTrue, 0, "until true, performance")
 CheckInit(iWhileTrue, 3, "while true")
 CheckInit(iUntilFalse, 3, "until false")

 CheckInit(gWhileFalse, 0, "while false in global code")
 CheckInit(gUntilTrue, 0, "until true in global code")
 CheckInit(gWhileTrue, 3, "while true in global code")
 CheckInit(gUntilFalse, 3, "until false in global code")
 CheckInit(gUntilTruek, 0, "until truek in global code")
 CheckInit(gUntilFalsek, 3, "until falsek in global code")
endin

; truek and falsek are B: the body has one init pass, and loops each cycle
instr 2
 iWhileTruek, iUntilFalsek, iWhileFalsek, iUntilTruek init 0

 kCount = 0
 while truek do
  iWhileTruek += 1
  Limit(iWhileTruek, 1, "while truek")
  kCount += 1
  LimitK(kCount, "while truek")
  if kCount == 3 then
   break
  endif
 od
 Check(kCount, 3, "while truek")

 kCount = 0
 until falsek do
  iUntilFalsek += 1
  Limit(iUntilFalsek, 1, "until falsek")
  kCount += 1
  LimitK(kCount, "until falsek")
  if kCount == 3 then
   break
  endif
 od
 Check(kCount, 3, "until falsek")

 kCount = 0
 while falsek do
  iWhileFalsek += 1
  Limit(iWhileFalsek, 1, "while falsek")
  kCount += 1
  LimitK(kCount, "while falsek")
 od
 Check(kCount, 0, "while falsek")

 kCount = 0
 until truek do
  iUntilTruek += 1
  Limit(iUntilTruek, 1, "until truek")
  kCount += 1
  LimitK(kCount, "until truek")
 od
 Check(kCount, 0, "until truek")

 CheckInit(iWhileTruek + iUntilFalsek + iWhileFalsek + iUntilTruek, 4,
           "truek and falsek loops, init")
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
