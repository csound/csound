<CsTest>
description = "a member written into a struct array's element in place keeps the order, branches and other members of a copy"

[expect]
exit = 0
output = ["every member written in place checked", "a first statement checked"]
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

struct Box gain:k
struct Wide gain:k, level:k, on:B, name:S, values:k[], sig:a
struct Named name:S
struct Holder inner:Box
struct IBox value:i
struct Pair gain:k, level:k
gps@global:Pair[] init 2
gkState init 0
gkIndexCalls init 0
giState init 0
giIndexCalls init 0
gkSlot init 0
gkFirst init 3
gfs@global:Box[] init 2
struct Bank values:k[], level:k
gbs@global:Bank[] init 2

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

; writes another member of the element its result is written into
opcode Louder(kLevel:k):k
 kIndex = 1
 gps[kIndex].level = kLevel
 xout 0.5
endop

; an index and a value with side effects (#3703): the index runs first
opcode KIndex():k
 gkState = 10
 gkIndexCalls += 1
 xout 1
endop

opcode KValue():k
 xout gkState
endop

opcode IIndex():i
 giState = 10
 giIndexCalls += 1
 xout 1
endop

opcode IValue():i
 xout giState
endop

; a value that moves the index variable it is written at (0 at init)
opcode Bump():k
 gkSlot = 1
 xout gkSlot * 5
endop

opcode Fill(kLevel:k):k[]
 kIndex = 1
 gbs[kIndex].level = kLevel
 kOut:k[] init 2
 kOut[1] = kLevel * 2
 xout kOut
endop

instr 1
 kc = timeinstk()
 kIndex = 1

 ; a constant written in each cycle, then changed at a k index (#3695)
 boxes:Box[] init 2
 boxes[1].gain = 0.5
 boxes[kIndex].gain = boxes[kIndex].gain * 0.5
 Check(boxes[kIndex].gain, 0.25, "a constant, then a write at a k index")

 ; the writes of one cycle land in their order
 os:Box[] init 2
 os[1].gain = 1
 kFirst = os[kIndex].gain
 os[1].gain = 2
 kSecond = os[kIndex].gain
 Check(kFirst, 1, "the first of two writes")
 Check(kSecond, 2, "the second of two writes")

 ; a write the value's call makes to another member of the element stays
 gps[1].gain = Louder(kc)
 Check(gps[kIndex].gain, 0.5, "the member written")
 Check(gps[kIndex].level, kc, "a member the value's call writes")

 ; the same for an array member
 gbs[1].values = Fill(kc)
 filled:k[] init 2
 filled = gbs[kIndex].values
 Check(filled[1], kc * 2, "an array member")
 Check(gbs[kIndex].level, kc, "a member the array value's call writes")

 ; an index that calls a function is computed before the value, once, at a
 ; k and an i index
 gkState = 0
 gkIndexCalls = 0
 ks:Box[] init 2
 ks[KIndex()].gain = KValue()
 Check(ks[kIndex].gain, 10, "a value read after a k index's call")
 Check(gkIndexCalls, 1, "the k index's calls")
 is2:Box[] init 2
 is2[IIndex()].gain = IValue()
 CheckInit(i(is2[1].gain), 10, "a value read after an i index's call, at init")
 Check(is2[kIndex].gain, 10, "a value read after an i index's call")
 CheckInit(giIndexCalls, 1, "the i index's calls")
 ; and so is a call in an array read as the index
 slots:k[] fillarray 0, 1
 gkState = 0
 as2:Box[] init 2
 as2[slots[KIndex()]].gain = KValue()
 Check(as2[kIndex].gain, 10, "a value read after a call in an index's array read")
 ; a variable is read when the member is written, as the copy back read it
 gkSlot = 0
 vs:Box[] init 2
 vs[gkSlot].gain = Bump()
 Check(vs[1].gain, 5, "a variable index the value moves")
 Check(vs[0].gain, 0, "the element it moved from")

 ; a constant in a branch taken in performance
 bs:Box[] init 2
 if kc == 2 then
  bs[1].gain = 0
 else
  bs[1].gain = kc * 10
 endif
 Check(bs[kIndex].gain, kc == 2 ? 0 : kc * 10, "a constant in a branch")

 ; the other members of the element are left as they are
 start:k[] fillarray 3, 4
 ws:Wide[] init 2
 ws[kIndex].values = start
 ws[kIndex].name = "kept"
 ws[kIndex].level = kc
 ws[1].gain = 0.5
 ws[1].on = kc > 2
 asig = a(kc)
 ws[1].sig = asig
 Check(ws[kIndex].gain, 0.5, "the member written")
 Check(ws[kIndex].level, kc, "a member written at a k index beside it")
 Check(ws[kIndex].on ? 1 : 0, kc > 2 ? 1 : 0, "a B member")
 Check(k(ws[kIndex].sig), k(asig), "an a member")
 Check(strcmpk(ws[kIndex].name, "kept"), 0, "an S member left alone")
 kept:k[] init 2
 kept = ws[kIndex].values
 Check(kept[0] + kept[1], 7, "a k[] member left alone")

 ; at a k index, an S and an i member
 ns:Named[] init 2
 ns[kIndex].name = sprintfk("n%d", kc)
 Check(strcmpk(ns[kIndex].name, sprintfk("n%d", kc)), 0, "an S member at a k index")

 ib:IBox[] init 2
 ib[kIndex].value = kc
 Check(ib[kIndex].value, kc, "an i member at a k index")

 ; a member of a nested struct, at an i and at a k index
 hs:Holder[] init 2
 hs[1].inner.gain = kc
 Check(hs[kIndex].inner.gain, kc, "a member of a nested struct")
 hk:Holder[] init 2
 hk[kIndex].inner.gain = kc * 2
 Check(hk[kIndex].inner.gain, kc * 2, "a member of a nested struct at a k index")

 ; a value written at init lands at init
 is:Box[] init 2
 is[0].gain = 5
 CheckInit(i(is[0].gain), 5, "a write at init")

 if kc == 4 then
  printks("every member written in place checked\n", 0)
 endif
endin

; a member written by the first statement of an instrument's body keeps
; what its value's expression is computed by
instr 2
 gfs[1].gain = gkFirst * 2
 kIndex = 1
 Check(gfs[kIndex].gain, 6, "a member written by a body's first statement")
 printks("a first statement checked\n", 0)
 turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.04
i 2 0 0.04
e
</CsScore>
</CsoundSynthesizer>
