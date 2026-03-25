<CsoundSynthesizer>
<CsOptions>
-o dac
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 8
0dbfs = 1

pos_func@global:i = ftgen:i(0, 0, 16, -2, 0, 0, 45, 0, 90, 0, 135, 0, 180, 0, 225, 0, 270, 0, 315, 0)
w_func@global:i = ftgen:i(0, 0, 8, -2, 1, 1, 0, 1, 1, 1, 1, 1)

// Audio test
instr 1
    time:k = timeinsts()

    asig = poscil(1, 440)
    source:k[] = [phasor:k(0.3) * 360, 0.0]
    lpos:i[][] = init(8, 2)
    lpos = [0, 0, 45, 0, 90, 0, 135, 0, 180, 0, 225, 0, 270, 0, 315, 0]

    dbap_sig:a[] = init(8)
    dbap_sig = dbap(asig, 1, source, lpos, 3, 24.0) // using arr pos

    rms_check:k = 0
    for ch in dbap_sig do
        rms_check += rms(ch)
    od

    if (rms_check <= 0.0 && time > 0.01) then
        printf("[FAIL] Audio test failed\n", 1)
        schedulek("ExitError", 0, 1)
    endif

    out(dbap_sig)

    if time > 2 then
        printf("[PASS] Audio test passed without errors\n", 1)
        turnoff
    endif

endin

// Position test
instr 2
    dbap_gains:k[] = init(8)

    lpos:i[][] = init(8, 2)
    lpos = [0, 0, 45, 0, 90, 0, 135, 0, 180, 0, 225, 0, 270, 0, 315, 0]

    azi:i[] = [0, 45, 90, 135, 180, 225, 270, 315]
    ndx_count:k = init(0)
    source:k[] = [azi[ndx_count], 0.0]
    dbap_gains = dbapgains:k[](1, source, lpos, 3, 24.0)

    for g, i in dbap_gains do
        if (i != ndx_count) then
            if (g > dbap_gains[ndx_count]) then
                printf("[FAIL] Position test failed, gain at channel %d should be greater because the source is located there\n", 1, ndx_count)
                schedulek("ExitError", 0, 1)
            endif
            if (g < 0.0) then
                printf("[FAIL] Position test failed, gain should be positive\n", 1, ndx_count)
                schedulek("ExitError", 0, 1)
            endif
        endif
    od

    ndx_count += 1
    if (ndx_count >= 8) then
        printf("[PASS] Position test passed without errors\n", 1)
        turnoff
    endif

endin

// Weights test
instr 3
    source:k[] = fillarray(90, 0)

    gains:k[] = init(8)
    gains = dbapgains(1, source, pos_func, 3, 24.0, 2, w_func)

    if gains[2] > 0.0 then
        printf("[FAIL] Weights test failed, gain at channel 3 should be zero\n", 1)
        schedulek("ExitError", 0, 1)
    endif

    printf("[PASS] Weights test passed without errors\n", 1)
    turnoff

endin

// Center test
instr 4
    source:k[] = [0, 0, 0]

    lpos:i[][] = init(4, 3)
    lpos = [1, 45, 0, 1, 135, 0, 1, -45, 0, 1, -135, 0] // 3D

    gains:k[] = init(4)
    gains = dbapgains:k[](1, source, lpos, 3, 24.0)

    for g in gains do
        if (g != 0.5) then
            printf("[FAIL] Center source test failed, gain should be 0.5 for each channel\n", 1)
            exitnowk(-1)
        endif
    od

    printf("[PASS] Center source test passed without errors\n", 1)
    printf("[DONE] All tests: OK\n", 1)
    exitnowk(0)

endin


</CsInstruments>
<CsScore>

i 1 0 3
i 2 3 1
i 3 4 1
i 4 5 1


</CsScore>
</CsoundSynthesizer>
