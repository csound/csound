; Test for named instrument ramps
; Related to issue: https://github.com/csound/csound/issues/2201
; Fixed by PR: https://github.com/csound/csound/pull/2335

<CsoundSynthesizer>
<CsOptions>
-nd -m134
</CsOptions>
<CsInstruments>

sr = 48000
ksmps = 64
nchnls = 2
0dbfs = 1

#include "libassert.orc"

instr 1
    printf_i "p4 => %f\n", 1, p4

    if(p5 > 0) then
        assertEquals(p4, p5)
    endif
endin

instr meh
    printf_i "p4 => %f\n", 1, p4
    if(p5 > 0) then
        assertEquals(p4, p5)
    endif
endin

</CsInstruments>
<CsScore>
i1 0 0.1 42 0
i1 + 0.1 < 43
i1 + 0.1 44 0

i "meh" 1 0.1 42 0
i "meh" + 0.1 < 43
i "meh" + 0.1 44 0
</CsScore>
</CsoundSynthesizer>
