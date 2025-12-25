<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: Same module imported with different aliases
   This verifies that the same module can be aliased differently
   and both aliases work correctly to access the same underlying data. */

import "alias_module_a.orc" as modA
import "alias_module_a.orc" as altA

instr 1
    /* Access the same module variable through different aliases */
    iVal1 = modA.giModuleA_Value
    iVal2 = altA.giModuleA_Value
    
    prints "modA.giModuleA_Value = %d\n", iVal1
    prints "altA.giModuleA_Value = %d\n", iVal2
    
    /* Both should have the same value (100) */
    if (iVal1 == 100 && iVal2 == 100) then
        prints "PASS: Same module accessible via different aliases\n"
    else
        prints "FAIL: Expected both to be 100, got %d and %d\n", iVal1, iVal2
        exitnow 1
    endif
    
    /* Use UDOs from the module */
    iDouble1 ModuleA_Double 5
    prints "ModuleA_Double(5) = %d\n", iDouble1
    
    if (iDouble1 == 10) then
        prints "PASS: UDO works correctly\n"
    else
        prints "FAIL: Expected 10, got %d\n", iDouble1
        exitnow 1
    endif
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
