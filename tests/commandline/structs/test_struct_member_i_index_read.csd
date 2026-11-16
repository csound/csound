<CsTest>
description = "a k, B or a member of a struct array's element read at an i index follows it in performance"

[expect]
exit = 0
output = ["every member read at an i index checked"]
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
struct BFlag on:B
struct Sig sig:a
struct KValues values:k[]
struct Outer inner:KBox, count:i

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

; each element is written at a k index in every cycle, and read at an i index
instr 1
 kc = timeinstk()
 kIndex = 1

 ks:KBox[] init 2
 ks[kIndex].value = kc
 Check(ks[1].value, kc, "a k member")
 kValue = ks[1].value
 Check(kValue, kc, "a k member into a k")

 fs:BFlag[] init 2
 fs[kIndex].on = kc > 2
 Check(fs[1].on ? 1 : 0, kc > 2 ? 1 : 0, "a B member as a ternary's condition")
 on:B = fs[1].on
 Check(on ? 1 : 0, kc > 2 ? 1 : 0, "a B member into a B")
 kThen init 0
 kWant init 0
 if fs[1].on then
  kThen += 1
 endif
 kWant += kc > 2 ? 1 : 0
 Check(kThen, kWant, "a B member as an if's condition")

 as:Sig[] init 2
 asig = a(kc)
 as[kIndex].sig = asig
 Check(k(as[1].sig), k(asig), "an a member")

 os:Outer[] init 2
 os[kIndex].inner.value = kc
 Check(os[1].inner.value, kc, "a k member of a nested struct")

 kv:k[] init 2
 kv[1] = kc
 v0:KValues init kv
 vs:KValues[] init 2
 vs[1] = v0
 vs[kIndex].values = kv
 Check(vs[1].values[1], kc, "an element of a k[] member")

 ; written at init at an i index, read at init
 is:KBox[] init 2
 is[0].value = 5
 CheckInit(i(is[0].value), 5, "a k member at init")

 if kc == 4 then
  printks("every member read at an i index checked\n", 0)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
e
</CsScore>
</CsoundSynthesizer>
