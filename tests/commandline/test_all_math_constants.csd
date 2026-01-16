<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
    ; Test that all math constants are available
    prints "=== Math Constants Test ===\n"

    ; Existing constants
    prints "M_E: %g\n", $M_E
    prints "M_PI: %g\n", $M_PI
    prints "M_LOG2E: %g\n", $M_LOG2E
    prints "M_LN2: %g\n", $M_LN2
    prints "M_SQRT2: %g\n", $M_SQRT2
    prints "M_INF: %g\n", $M_INF

    ; New constants
    prints "M_MAX_VALUE: %g\n", $M_MAX_VALUE
    prints "M_MIN_VALUE: %g\n", $M_MIN_VALUE

    ; Verify they're reasonable
    if ($M_MAX_VALUE <= 0 || $M_MIN_VALUE <= 0) then
        prints "FAIL: MAX_VALUE and MIN_VALUE should be positive\n"
        exitnow 1
    endif

    if ($M_MIN_VALUE >= $M_MAX_VALUE) then
        prints "FAIL: MIN_VALUE should be smaller than MAX_VALUE\n"
        exitnow 1
    endif

    ; Test that they work in expressions with other constants
    itest1 = $M_PI * $M_MAX_VALUE
    itest2 = $M_E + $M_MIN_VALUE

    ; Note: PI * MAX_VALUE will likely overflow to infinity
    if (itest1 < $M_MAX_VALUE && itest1 != $M_INF) then
        prints "FAIL: PI * MAX_VALUE should be >= MAX_VALUE or infinity\n"
        exitnow 1
    endif

    ; Note: E + MIN_VALUE might equal E due to floating-point precision
    ; This is expected when adding very small to much larger numbers
    if (itest2 < $M_E) then
        prints "FAIL: E + MIN_VALUE should not be smaller than E\n"
        exitnow 1
    endif

    prints "PASS: All math constants work correctly together\n"
    exitnow 0
endin

</CsInstruments>
<CsScore>
i1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
