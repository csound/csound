<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

import "module_a.orc"
import "module_b.orc"

instr 1
    ; Test that each module's UDO sees its own variable values
    ; Both g-prefix and @global syntax
    iValA_g, iValA_at = ModuleATest()  ; Should return 100, 150
    iValB_g, iValB_at = ModuleBTest()  ; Should return 200, 250

    prints "Module A - giSharedName: %d, sharedValue: %d\n", iValA_g, iValA_at
    prints "Module B - giSharedName: %d, sharedValue: %d\n", iValB_g, iValB_at

    ; Verify they're different (namespace isolation working)
    if (iValA_g != 100) then
        prints "FAIL: Expected iValA_g = 100, got %d\n", iValA_g
        exitnow 1
    endif
    if (iValA_at != 150) then
        prints "FAIL: Expected iValA_at = 150, got %d\n", iValA_at
        exitnow 1
    endif
    if (iValB_g != 200) then
        prints "FAIL: Expected iValB_g = 200, got %d\n", iValB_g
        exitnow 1
    endif
    if (iValB_at != 250) then
        prints "FAIL: Expected iValB_at = 250, got %d\n", iValB_at
        exitnow 1
    endif
    prints "PASS: Namespace isolation working for both g-prefix and @global\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
