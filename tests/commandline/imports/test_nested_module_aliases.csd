<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: Nested module imports with aliases
   - CSD imports inner module as moduleA
   - CSD imports outer module as moduleB  
   - Outer module (moduleB) imports inner module as "inner"
   
   This tests that aliases work correctly across module boundaries
   and that the same module imported at different levels with different
   aliases all resolve correctly. */

import "nested_alias_inner.orc" as moduleA
import "nested_alias_outer.orc" as moduleB

instr 1
    prints "\n=== Test: Nested Module Aliases ===\n\n"
    
    /* Test 1: Access inner module variable directly from CSD via alias */
    iInnerVal = moduleA.giInnerValue
    prints "Test 1: moduleA.giInnerValue = %d (expected 500)\n", iInnerVal
    if (iInnerVal != 500) then
        prints "FAIL: Expected 500\n"
        exitnow 1
    endif
    prints "PASS: Direct inner module access works\n\n"
    
    /* Test 2: Access outer module variable via alias */
    iOuterVal = moduleB.giOuterValue
    prints "Test 2: moduleB.giOuterValue = %d (expected 1000)\n", iOuterVal
    if (iOuterVal != 1000) then
        prints "FAIL: Expected 1000\n"
        exitnow 1
    endif
    prints "PASS: Outer module access works\n\n"
    
    /* Test 3: Call inner module's UDO (should be in global namespace) */
    iDoubled InnerDouble 7
    prints "Test 3: InnerDouble(7) = %d (expected 14)\n", iDoubled
    if (iDoubled != 14) then
        prints "FAIL: Expected 14\n"
        exitnow 1
    endif
    prints "PASS: Inner UDO callable from CSD\n\n"
    
    /* Test 4: Call outer module's UDO that reads inner's variable via alias */
    iFromOuter OuterGetInnerValue
    prints "Test 4: OuterGetInnerValue() = %d (expected 500)\n", iFromOuter
    if (iFromOuter != 500) then
        prints "FAIL: Expected 500\n"
        exitnow 1
    endif
    prints "PASS: Outer UDO can read inner module var via alias\n\n"
    
    /* Test 5: Call outer module's UDO that uses inner's UDO */
    iTripled OuterTriple 10
    prints "Test 5: OuterTriple(10) = %d (expected 30)\n", iTripled
    if (iTripled != 30) then
        prints "FAIL: Expected 30\n"
        exitnow 1
    endif
    prints "PASS: Outer UDO can call inner UDO\n\n"
    
    /* Test 6: Call outer module's UDO that computes with inner's variable */
    iModified OuterModifyInner 100
    prints "Test 6: OuterModifyInner(100) = %d (expected 600)\n", iModified
    if (iModified != 600) then
        prints "FAIL: Expected 600\n"
        exitnow 1
    endif
    prints "PASS: Outer UDO computation with inner var works\n\n"
    
    prints "=== All nested alias tests PASSED ===\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
