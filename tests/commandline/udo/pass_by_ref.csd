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


instr 1
    iv = 33
    iv2 = 77

    print iv
    incr(iv)
    print iv

    print iv2
    iv3 = incr2(iv2)
    print iv2, iv3

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

    print(factorial(4))
    assertEquals(1, factorial(1))
    assertEquals(2, factorial(2))
    assertEquals(6, factorial(3))
    assertEquals(24, factorial(4))

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


</CsInstruments>
<CsScore>
i1 0 1
; i"SoundTest" 0 4 220 0.25
; i"SoundTest" 1 3 330 0.25
; i"SoundTest" 2 3 440 0.25

</CsScore>
</CsoundSynthesizer>

