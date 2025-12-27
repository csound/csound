<CsoundSynthesizer>
<CsOptions>
-odac -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

; Import only giVar1, giVar3, GetValue1, and DoubleIt from the module
; (giVar2, giVar4, GetValue2, and TripleIt should NOT be available)
from "selective_test_module.orc" import giVar1, giVar3, GetValue1, DoubleIt

; Define a unique local variable to verify we can still define our own globals
giMyLocalVar = 999

instr 1
    ; Test imported variables
    prints "giVar1 = %d\\n", giVar1
    prints "giVar3 = %d\\n", giVar3

    ; Test that our local variable works
    prints "giMyLocalVar = %d\\n", giMyLocalVar

    ; Test imported UDOs
    iVal1 = GetValue1()
    prints "GetValue1() = %d\\n", iVal1

    iDoubled = DoubleIt(50)
    prints "DoubleIt(50) = %d\\n", iDoubled

    ; Verify all values
    if (giVar1 != 100) then
        prints "FAIL: Expected giVar1 = 100, got %d\\n", giVar1
        exitnow 1
    endif
    if (giVar3 != 300) then
        prints "FAIL: Expected giVar3 = 300, got %d\\n", giVar3
        exitnow 1
    endif
    if (giMyLocalVar != 999) then
        prints "FAIL: Expected giMyLocalVar = 999, got %d\\n", giMyLocalVar
        exitnow 1
    endif
    if (iVal1 != 1000) then
        prints "FAIL: Expected GetValue1() = 1000, got %d\\n", iVal1
        exitnow 1
    endif
    if (iDoubled != 100) then
        prints "FAIL: Expected DoubleIt(50) = 100, got %d\\n", iDoubled
        exitnow 1
    endif
    prints "PASS: Selective import - only listed items available\\n"
    turnoff
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
