<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

sr = 48000
ksmps	= 64
nchnls	= 2
0dbfs	= 1

#include "../libassert.orc"

struct TestStruct member1:i
struct TestStruct2515 var1:i, var2:i

// increments the value in the passed-in pointer
opcode incr(ival):void
    print ival
    ival += 1
    print ival
endop

// reads the value in the passed-in pointer
// writes value to passed-in output pointer
opcode incr2(ival):i
    ;ival += 1
    print ival
    ival += 2
    print ival
    iout = 22
    xout iout
    ; FIXME: does not work with constants, check for pfields and other args
    ; xout 22
endop

opcode factorial(icount):i
    iout = (icount <= 1) ? 1 : icount * factorial(icount - 1)
    xout iout
endop

opcode incrArray(iArr[]):void
    iArr[0] += 10
    iArr[1] += 20
endop


opcode incrK(kval):void
    kval += 3
endop


// k-rate direct pass-through; += 0 ensures a perf-time body.
opcode passK(kval):k
    kval += 0
    xout kval
endop


// k-rate pass-through with source mutation and return.
opcode incrAndReturnK(kval):k
    kval += 1
    xout kval
endop


opcode incrExpr(ival):(i,i)
    xout ival + 1, ival + 2
endop


// mixed xin pass-through and expression xout.
opcode mixedPassExpr(ival):(i,i)
    ival += 1
    xout ival, ival + 1
endop


opcode readStructMember(var:TestStruct2515):i
    xout var.var1
endop


// struct member pass-through with source writeback.
opcode incrStructMemberReturn(var:TestStruct2515):i
    var.var1 += 2
    xout var.var1
endop


// UDO that modifies its input in-place and returns it via xout.
// Tests pass-by-reference with constant inputs: xout of a xin variable
// should not corrupt the global constant pool.
opcode incrAndReturn(ival):i
    ival += 1
    xout ival
endop


// array pass-through with source writeback.
opcode incrArrayReturn(iArr[]):i[]
    iArr[0] += 10
    iArr[1] += 20
    xout iArr
endop


// array element xout is not a pass-through alias.
opcode firstArrayValue(iArr[]):i
    xout iArr[0]
endop


// audio block pass-through with source writeback.
opcode scaleAudio(aSig):a
    aSig *= 0.5
    xout aSig
endop


instr 1
    iv = 33
    iv2 = 77

    print iv
    incr(iv)
    print iv

    print iv2
    iv3 = incr2(iv2)
    print iv2, iv3

    iExpr1, iExpr2 = incrExpr(iv)

    ; Test pass-by-reference: iv should be modified from 33 to 34
    if(iv != 34) then
        prints("ERROR: Pass-by-reference failed for incr(). iv was %g, expected 34\n", iv)
        exitnow(-1)
    endif

    ; Test pass-by-reference: iv2 should be modified from 77 to 79
    if(iv2 != 79) then
        prints("ERROR: Pass-by-reference failed for incr2(). iv2 was %g, expected 79\n", iv2)
        exitnow(-1)
    endif

    ; Test return value: iv3 should be 22
    if(iv3 != 22) then
        prints("ERROR: Return value failed for incr2(). iv3 was %g, expected 22\n", iv3)
        exitnow(-1)
    endif

    if(iExpr1 != 35 || iExpr2 != 36) then
        prints("ERROR: xout expression failed for incrExpr(). iExpr1=%g, iExpr2=%g\n", iExpr1, iExpr2)
        exitnow(-1)
    endif

    print(factorial(4))
    assertEquals(1, factorial(1))
    assertEquals(2, factorial(2))
    assertEquals(6, factorial(3))
    assertEquals(24, factorial(4))

endin


instr 2
    iArr[] init 2
    iArr[0] = 1
    iArr[1] = 2

    incrArray(iArr)

    if (iArr[0] != 11 || iArr[1] != 22) then
        prints("ERROR: Pass-by-reference failed for incrArray(). iArr[0]=%g, iArr[1]=%g\n", iArr[0], iArr[1])
        exitnow(-1)
    endif
endin


instr 3
    ts:TestStruct init 10

    incr(ts.member1)

    if (ts.member1 != 11) then
        prints("ERROR: Pass-by-reference failed for TestStruct. member1=%g\n", ts.member1)
        exitnow(-1)
    endif
endin


instr 4
    ts:TestStruct2515 init 1, 1
    iVal = readStructMember(ts)

    assertEquals(iVal, 1)
endin


instr 20
// k-rate input mutation across perf cycles.
kVal init 10
incrK(kVal)
if (timeinstk() == 1) then
    if (kVal != 13) then
        printks "ERROR: incrK cycle 1 kVal=%g\n", 0, kVal
        exitnowk(-1)
    endif
endif
if (timeinstk() == 2) then
    if (kVal != 16) then
        printks "ERROR: incrK cycle 2 kVal=%g\n", 0, kVal
        exitnowk(-1)
    endif
endif
endin


instr 21
// k-rate pass-through seed and mutable source writeback.
kSrc init 5
kOut = incrAndReturnK(kSrc)
if (timeinstk() == 1) then
    if (kSrc != 6 || kOut != 6) then
        printks "ERROR: incrAndReturnK cycle 1 kSrc=%g kOut=%g\n", 0, kSrc, kOut
        exitnowk(-1)
    endif
endif
if (timeinstk() == 2) then
    if (kSrc != 7 || kOut != 7) then
        printks "ERROR: incrAndReturnK cycle 2 kSrc=%g kOut=%g\n", 0, kSrc, kOut
        exitnowk(-1)
    endif
endif
endin


instr 22
// k-rate direct pass-through follows changing source values.
kSrc init 8
kOut = passK(kSrc)
if (timeinstk() == 1) then
    if (kOut != 8) then
        printks "ERROR: passK cycle 1 kSrc=%g kOut=%g\n", 0, kSrc, kOut
        exitnowk(-1)
    endif
endif
kSrc += 1
if (timeinstk() == 2) then
    if (kOut != 9) then
        printks "ERROR: passK cycle 2 kSrc=%g kOut=%g\n", 0, kSrc, kOut
        exitnowk(-1)
    endif
endif
endin


instr 23
// audio pass-through copies full blocks and writes back source.
aSrc init 0.5
aOut = scaleAudio(aSrc)
kSrc downsamp aSrc
kOut downsamp aOut
if (timeinstk() > 0) then
    if (abs(kSrc - 0.25) > 0.000001) then
        printks "ERROR: scaleAudio source kSrc=%g\n", 0, kSrc
        exitnowk(-1)
    endif
    if (abs(kOut - 0.25) > 0.000001) then
        printks "ERROR: scaleAudio output kOut=%g\n", 0, kOut
        exitnowk(-1)
    endif
    turnoff
endif
endin


instr 24
// array pass-through xout and array element xout.
iArr[] fillarray 1, 2
iOut[] = incrArrayReturn(iArr)
assertEquals(iArr[0], 11)
assertEquals(iArr[1], 22)
assertEquals(iOut[0], 11)
assertEquals(iOut[1], 22)
iElem = firstArrayValue(iOut)
assertEquals(iElem, 11)
endin


instr 25
// mixed pass-through and expression outputs.
src:i = 10
pass:i, expr:i = mixedPassExpr(src)
assertEquals(src, 11)
assertEquals(pass, 11)
assertEquals(expr, 12)
endin


instr 26
// struct member xout writes back only that member.
ts:TestStruct2515 init 3, 4
iVal = incrStructMemberReturn(ts)
assertEquals(ts.var1, 5)
assertEquals(ts.var2, 4)
assertEquals(iVal, 5)
endin


opcode sound(iamp, ifreq):a
    aout = oscili(iamp, ifreq)
    if(ifreq < sr/2) then
        aout += sound(iamp / 2, ifreq * 2)
    endif

    xout aout
endop

instr SoundTest
    aout = sound(p5, p4)
    outs aout, aout
endin

; schedule("SoundTest", 0, 4)


instr 10
// Test incrAndReturn with a literal constant: should not corrupt
// the global constant pool and should produce the correct result.
val:i = incrAndReturn(1)
assertEquals(val,2)
print val
// Test with a variable to verify pass-through still works.
val:i = incrAndReturn(val)
assertEquals(val,3)
print val
// Test distinct input/output variables: the input should still be modified
// by pass-by-reference, while the output receives the returned value.
src:i = 5
dst:i = incrAndReturn(src)
assertEquals(src,6)
assertEquals(dst,6)
print src, dst
// Test p-field input: return value changes, p-field remains read-only.
pval:i = incrAndReturn(p4)
assertEquals(pval,8)
assertEquals(p4,7)
pval2:i = incrAndReturn(p4)
assertEquals(pval2,8)
assertEquals(p4,7)
endin


</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
i3 0 1
i4 0 1
i20 0 0.01
i21 0 0.01
i22 0 0.01
i23 0 0.1
i24 0 1
i25 0 1
i26 0 1
i10 0 1 7
; i"SoundTest" 0 4 220 0.25
; i"SoundTest" 1 3 330 0.25
; i"SoundTest" 2 3 440 0.25

</CsScore>
</CsoundSynthesizer>
