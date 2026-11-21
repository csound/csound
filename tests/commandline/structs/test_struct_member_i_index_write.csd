<CsTest>
description = "a value written into a k or B member of a struct array's element at an i index lands in performance"

[expect]
exit = 0
output = ["every member written at an i index checked"]
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

struct KBox value:k
struct Pair first:k, second:k
struct BFlag on:B
struct Outer inner:KBox, count:i
struct Point x:k
struct Shape points:Point[]

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

; each member is written at an i index in every cycle, and read back at a
; k index and at an i index
instr 1
 kc = timeinstk()
 kIndex = 1

 ks:KBox[] init 2
 ks[1].value = kc
 Check(ks[kIndex].value, kc, "a k member, read at a k index")
 Check(ks[1].value, kc, "a k member, read at an i index")
 Check(ks[0].value, 0, "the other element")

 ; two members of one element, written one after the other
 ps:Pair[] init 2
 ps[1].first = kc
 ps[1].second = kc * 2
 Check(ps[kIndex].first, kc, "the first of two members")
 Check(ps[kIndex].second, kc * 2, "the second of two members")

 fs:BFlag[] init 2
 fs[1].on = kc > 2
 Check(fs[kIndex].on ? 1 : 0, kc > 2 ? 1 : 0, "a B member")

 os:Outer[] init 2
 os[1].inner.value = kc
 Check(os[kIndex].inner.value, kc, "a k member of a nested struct")

 ; a write into an element of an array member is left at init, as before,
 ; and does not stop the note
 point:Point init 1
 points:Point[] init 2
 points[1] = point
 shape:Shape init points
 shapes:Shape[] init 2
 shapes[1] = shape
 shapes[1].points[1].x = kc

 ; a constant is written in performance too, as kValue = 0 is: in a
 ; branch taken in performance, it lands
 rs:KBox[] init 2
 if kc == 2 then
  rs[1].value = 0
 else
  rs[1].value = kc * 10
 endif
 Check(rs[kIndex].value, kc == 2 ? 0 : kc * 10, "a constant in a branch")

 ; a value written at init still lands at init
 is:KBox[] init 2
 is[0].value = 5
 CheckInit(i(is[0].value), 5, "a k member at init")

 if kc == 4 then
  printks("every member written at an i index checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
