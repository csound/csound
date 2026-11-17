<CsTest>
description = "an element of an array member of a struct array's element read at a k index follows the index"

[expect]
exit = 0
output = ["every array member read at a k index checked"]
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
struct Values values:k[], counts:i[]
struct Point x:k, on:b
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

; kIndex is 1 in odd cycles, where every element read is true or from the
; second struct
instr 1
 kc = timeinstk()
 kIndex = kc % 2

 offs:b[] fillarray false, false
 ons:b[] fillarray true, true
 off:Flags init offs
 on:Flags init ons
 fs:Flags[] init 2
 fs[0] = off
 fs[1] = on

 value:B = fs[kIndex].flags[1]
 Check(value ? 1 : 0, kIndex, "a b[] member's element into a B")
 Check(fs[kIndex].flags[0] ? 1 : 0, kIndex, "as a ternary's condition")
 both:B = fs[kIndex].flags[0] && true
 Check(both ? 1 : 0, kIndex, "in &&")
 kThen init 0
 kWant init 0
 if fs[kIndex].flags[0] then
  kThen += 1
 endif
 kWant += kIndex
 Check(kThen, kWant, "as an if's condition")
 kJ = 1 - kIndex
 Check(fs[kIndex].flags[kJ] ? 1 : 0, kIndex, "at two k indices")

 k0:k[] fillarray 10, 11
 k1:k[] fillarray 20, 21
 i0:i[] fillarray 30, 31
 i1:i[] fillarray 40, 41
 v0:Values init k0, i0
 v1:Values init k1, i1
 vs:Values[] init 2
 vs[0] = v0
 vs[1] = v1
 Check(vs[kIndex].values[1], 11 + 10 * kIndex, "a k[] member's element")
 Check(vs[kIndex].counts[0], 30 + 10 * kIndex, "an i[] member's element")

 near:Point init 1, false
 far:Point init 2, true
 nears:Point[] init 2
 fars:Point[] init 2
 nears[1] = near
 fars[1] = far
 small:Shape init nears
 large:Shape init fars
 shapes:Shape[] init 2
 shapes[0] = small
 shapes[1] = large
 point:Point = shapes[kIndex].points[1]
 Check(point.x, 1 + kIndex, "a struct array member's element")
 Check(shapes[kIndex].points[1].x, 1 + kIndex, "a k member of it")
 pointOn:B = shapes[kIndex].points[1].on
 Check(pointOn ? 1 : 0, kIndex, "a b member of it into a B")

 ; at an i index, the element is read at init
 CheckInit(fs[1].flags[0] ? 1 : 0, 1, "at an i index")

 if kc == 4 then
  printks("every array member read at a k index checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
