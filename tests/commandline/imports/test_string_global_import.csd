<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: import gS variable from module */
from "string_module.orc" import gSModuleMessage
import "string_module.orc" as sm

instr 1
    if (strcmp(gSModuleMessage, "Hello from module string") != 0) then
        prints "FAIL: Expected gSModuleMessage to match imported value\n"
        exitnow 1
    endif

    if (strcmp(sm.gSModuleMessage, "Hello from module string") != 0) then
        prints "FAIL: Expected sm.gSModuleMessage to match imported value\n"
        exitnow 1
    endif

    prints "PASS: gS string global import works\n"
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>

