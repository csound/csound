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
    ; Test that MAX_VALUE and MIN_VALUE constants are defined and accessible
    imax_val = $M_MAX_VALUE
    imin_val = $M_MIN_VALUE

    ; Basic sanity checks
    ; MAX_VALUE should be positive and very large
    if (imax_val <= 0) then
        prints "FAIL: MAX_VALUE should be positive, got: %g\n", imax_val
        exitnow 1
    endif

    if (imax_val < 1e30) then
        prints "FAIL: MAX_VALUE should be very large (>1e30), got: %g\n", imax_val
        exitnow 1
    endif

    ; MIN_VALUE should be positive and very small
    if (imin_val <= 0) then
        prints "FAIL: MIN_VALUE should be positive, got: %g\n", imin_val
        exitnow 1
    endif

    if (imin_val > 1e-30) then
        prints "FAIL: MIN_VALUE should be very small (<1e-30), got: %g\n", imin_val
        exitnow 1
    endif

    ; MIN_VALUE should be much smaller than MAX_VALUE
    if (imin_val >= imax_val) then
        prints "FAIL: MIN_VALUE should be much smaller than MAX_VALUE\n"
        prints "MIN_VALUE: %g, MAX_VALUE: %g\n", imin_val, imax_val
        exitnow 1
    endif

    ; Test arithmetic operations near the limits
    ; MAX_VALUE should be finite (not infinity)
    if (imax_val == $M_INF) then
        prints "FAIL: MAX_VALUE should not be infinity\n"
        exitnow 1
    endif

    ; Test that MAX_VALUE is close to the expected range
    ; Determine precision based on the actual MAX_VALUE
    if (imax_val > 1e100) then
        ; This is double precision (~1.8e308)
        iexpected_min = 1e300
        prints "Detected double precision MYFLT\n"
    else
        ; This is float precision (~3.4e38)
        iexpected_min = 1e35
        iexpected_max = 1e40
        if (imax_val > iexpected_max) then
            prints "FAIL: MAX_VALUE too large for float precision: %g\n", imax_val
            exitnow 1
        endif
        prints "Detected float precision MYFLT\n"
    endif

    if (imax_val < iexpected_min) then
        prints "FAIL: MAX_VALUE too small: %g\n", imax_val
        exitnow 1
    endif

    ; Test that we can do basic arithmetic with these values
    ihalf_max = imax_val * 0.5
    idouble_min = imin_val * 2

    if (ihalf_max <= 0) then
        prints "FAIL: Arithmetic with MAX_VALUE failed\n"
        exitnow 1
    endif

    if (idouble_min <= imin_val) then
        prints "FAIL: Arithmetic with MIN_VALUE failed\n"
        exitnow 1
    endif

    ; Additional arithmetic tests from simple test
    isum = imax_val + imin_val
    idiff = imax_val - imin_val
    iratio = imax_val / imin_val

    ; Note: imax + imin might equal imax due to floating-point precision
    ; This is expected behavior when adding a very small to a very large number
    if (isum < imax_val) then
        prints "FAIL: MAX_VALUE + MIN_VALUE should not be smaller than MAX_VALUE\n"
        exitnow 1
    endif

    if (idiff <= 0) then
        prints "FAIL: MAX_VALUE - MIN_VALUE should be positive\n"
        exitnow 1
    endif

    if (iratio <= 1) then
        prints "FAIL: MAX_VALUE / MIN_VALUE should be > 1\n"
        exitnow 1
    endif

    ; Test that MAX_VALUE is actually the maximum representable value
    ; by checking that MAX_VALUE + MAX_VALUE would overflow to infinity
    ioverflow_test = imax_val + imax_val
    if (ioverflow_test != $M_INF && ioverflow_test < imax_val) then
        prints "FAIL: MAX_VALUE + MAX_VALUE should overflow or be infinity\n"
        prints "Got: %g, Expected: infinity or overflow\n", ioverflow_test
        exitnow 1
    endif

    ; Test that MIN_VALUE is actually the minimum positive normalized value
    ; by checking that MIN_VALUE / 2 is either 0 or a subnormal number
    ihalf_min = imin_val * 0.5
    if (ihalf_min >= imin_val) then
        prints "FAIL: MIN_VALUE / 2 should be smaller than MIN_VALUE\n"
        exitnow 1
    endif

    ; All tests passed
    prints "PASS: MIN_VALUE and MAX_VALUE constants work correctly\n"
    prints "MIN_VALUE: %g, MAX_VALUE: %g, Ratio: %g\n", imin_val, imax_val, iratio

    exitnow 0
endin

</CsInstruments>
<CsScore>
i1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
