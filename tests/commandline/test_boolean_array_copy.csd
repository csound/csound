<CsTest>
description = "an = between b[] arrays copies at init, as for an i[], and in performance, and a for loop over a b[] reads its elements"

[expect]
exit = 0
output = ["every b[] copy checked"]
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

struct Flags flags:b[]

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

instr 1
 kc = timeinstk()
 flags:b[] fillarray true, false, true
 copy:b[] = flags
 CheckInit(lenarray(copy), 3, "the copy's length")
 CheckInit(copy[0] ? 1 : 0, 1, "the copy's first element")
 CheckInit(copy[1] ? 1 : 0, 0, "the copy's second element")
 CheckInit(copy[2] ? 1 : 0, 1, "the copy's third element")
 Check(copy[0] ? 1 : 0, 1, "the copy's first element in performance")

 ; the copy follows a write made in performance at a k index
 kIndex = 1
 written:b[] fillarray false, false
 if kc > 1 then
  written[kIndex] = true
 endif
 writtenCopy:b[] = written
 Check(writtenCopy[kIndex] ? 1 : 0, kc > 1 ? 1 : 0, "the copy after a write at a k index")

 ; a source with no value at init, a member read at a k index, is copied
 ; in performance
 offs:b[] fillarray false, false
 ons:b[] fillarray true, true
 off:Flags init offs
 on:Flags init ons
 holders:Flags[] init 2
 holders[0] = off
 holders[1] = on
 kHolder = kc % 2
 held:b[] = holders[kHolder].flags
 Check(held[kIndex] ? 1 : 0, kHolder, "a copy of a member read at a k index")

 grid:b[][] init 2, 2
 grid[1][0] = 1 == 1
 gridCopy:b[][] = grid
 CheckInit(gridCopy[1][0] ? 1 : 0, 1, "a b[][] copy")
 CheckInit(gridCopy[0][1] ? 1 : 0, 0, "a b[][] copy's false element")

 ; a for loop copies the array it goes over
 iPasses init 0
 iTrue init 0
 for element in flags do
  iPasses += 1
  iTrue += element ? 1 : 0
 od
 CheckInit(iPasses, 3, "passes over a b[]")
 CheckInit(iTrue, 2, "true elements of a b[]")

 iFirst init -1
 for element, iIndex in flags do
  if element && iFirst < 0 then
   iFirst = iIndex
  endif
 od
 CheckInit(iFirst, 0, "the first true element of a b[]")

 if kc == 3 then
  printks("every b[] copy checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.03
e
</CsScore>
</CsoundSynthesizer>
