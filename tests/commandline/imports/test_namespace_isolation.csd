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
    if (iValA_g == 100 && iValA_at == 150 && iValB_g == 200 && iValB_at == 250) then
        prints "SUCCESS: Namespace isolation working for both g-prefix and @global!\n"
    else
        prints "FAIL: Namespace isolation broken!\n"
        prints "  Expected: A(100,150) B(200,250)\n"
        prints "  Got: A(%d,%d) B(%d,%d)\n", iValA_g, iValA_at, iValB_g, iValB_at
    endif
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
