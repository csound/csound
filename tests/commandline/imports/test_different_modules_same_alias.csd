<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: Different modules imported with the same alias name
   This tests that alias names are scoped correctly.
   NOTE: This should likely be an error or the second import should shadow the first.
   Testing current behavior. */

import "alias_module_a.orc" as mod
import "alias_module_b.orc" as mod

instr 1
    /* Access module variable through the shared alias name.
       The second import should shadow the first, so we expect module B's value. */
    iVal = mod.giModuleB_Value

    prints "mod.giModuleB_Value = %d\n", iVal

    if (iVal == 200) then
        prints "PASS: Second import shadows first with same alias\n"
    else
        prints "FAIL: Expected 200, got %d\n", iVal
        exitnow 1
    endif

    /* Use UDO from module B (should work since it's imported) */
    iTriple ModuleB_Triple 5
    prints "ModuleB_Triple(5) = %d\n", iTriple

    if (iTriple == 15) then
        prints "PASS: Module B UDO works\n"
    else
        prints "FAIL: Expected 15, got %d\n", iTriple
        exitnow 1
    endif

    /* Module A's UDO should also still work (UDOs are in global namespace) */
    iDouble ModuleA_Double 5
    prints "ModuleA_Double(5) = %d\n", iDouble

    if (iDouble == 10) then
        prints "PASS: Module A UDO still accessible\n"
    else
        prints "FAIL: Expected 10, got %d\n", iDouble
        exitnow 1
    endif
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
