<CsoundSynthesizer>
<CsOptions>
-o /dev/null
</CsOptions>

<CsInstruments>
sr = 44100
ksmps = 10
nchnls = 2
0dbfs = 1

/* Test: Qualified UDO access using module.UDOName syntax */
import "simple_test.orc" as stm

instr 1
    /* First verify qualified variable access works */
    iVal = stm.giTestValue
    if (iVal != 42) then
        prints "FAIL: Expected stm.giTestValue = 42, got %d\n", iVal
        exitnow 1
    endif

    /* Call UDO using qualified syntax: module_alias.UDOName */
    aout stm.TestOsc 440

    /* Output for the test to verify audio was produced */
    outs aout, aout
endin
</CsInstruments>

<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
