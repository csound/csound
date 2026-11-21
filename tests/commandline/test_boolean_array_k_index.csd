<CsTest>
description = "an element of a b[] read at a k index is a B, and follows the index"

[expect]
exit = 0
output = ["every b[] element read at a k index checked"]
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

struct Holder flags:b[]

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

; With a b and a B overload, a value that changes in performance takes the B
opcode Rate(v:b):k
 xout 1
endop

opcode Rate(v:B):k
 xout 2
endop

; and the overload's output type follows
opcode Width(v:b):i
 xout 1
endop

opcode Width(v:B):k
 xout 2
endop

; bs[kIndex] is true in odd cycles, as kIndex is 1 there
instr 1
 kc = timeinstk()
 kIndex = kc % 2
 bs:b[] fillarray false, true
 yes:b = 1 < 2

 value:B = bs[kIndex]
 Check(value ? 1 : 0, kIndex, "into a B")
 Check(bs[kIndex] ? 1 : 0, kIndex, "as a ternary's condition")
 both:B = bs[kIndex] && yes
 Check(both ? 1 : 0, kIndex, "in && with a b")
 either:B = bs[kIndex] || false
 Check(either ? 1 : 0, kIndex, "in || with false")
 inverse:B = !bs[kIndex]
 Check(inverse ? 1 : 0, 1 - kIndex, "under !")
 Check(Rate(bs[kIndex]), 2, "the overload it takes")
 Check(Width(bs[kIndex]) + 0, 2, "an overload with another output type")
 holder:Holder init
 holder.flags fillarray false, true
 member:B = holder.flags[kIndex]
 Check(member ? 1 : 0, kIndex, "a b[] member of a struct")

 kAnd init 0
 kNot init 0
 kWant init 0
 if bs[kIndex] && yes then
  kAnd += 1
 endif
 if !bs[kIndex] then
  kNot += 1
 endif
 kWant += kIndex
 Check(kAnd, kWant, "if on && with a b")
 Check(kNot, kc - kWant, "if on !")

 ; writing at a k index still writes the element
 ws:b[] fillarray false, false
 ws[kIndex] = true
 Check(ws[kIndex] ? 1 : 0, 1, "written at a k index")
 Check(ws[1 - kIndex] ? 1 : 0, kc > 1 ? 1 : 0, "the other element")

 ; at an i index, the element is read at init, and stays a b
 CheckInit(bs[1] ? 1 : 0, 1, "at an i index")
 Check(Rate(bs[1]), 1, "the overload at an i index")

 if kc == 4 then
  printks("every b[] element read at a k index checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
