<CsoundSynthesizer>
<CsInstruments>

// Test break jumps to right target
instr 1
    sum:i = 0
    until (p4 < 0) do
        p4 = p4 - 1
        if (p4 == 2) then
            break
        endif
        sum = sum + p4
    od

    if (sum != 7) then
        exitnow(-1)
    endif
endin

// Test continue jumps to right target
instr 2
    sum:i = 0
    while (p4 > 0) do
        p4 = p4 - 1
        if (p4 == 2) then
            continue
        endif
        sum = sum + p4
    od

    if (sum != 8) then
        exitnow(-1)
    endif
endin

// Test break in for loop
instr 3
    sum:i = 0
    for ind in [1, 10, 100] do
        if (ind > 10) then
            break
        endif
        sum = sum + ind
    od

    if (sum != 11) then
        exitnow(-1)
    endif
endin

// Test continue in for loop
instr 4
    sum:i = 0
    for val, ind in [1, 10, 100, 1000] do
        if (ind == 2) then
            continue
        endif
        sum = sum + val
    od

    if (sum != 1011) then
        exitnow(-1)
    endif
endin

// Test nested loops
instr 5
    sum:i = 0
    for v1 in [1, 3, 5, 7] do
        for v2 in [2, 4, 6, 8] do
            rem:i = v2 % 4
            if (rem == 0) then
                continue
            endif
            sum = sum + v2
        od
        if (v1 > 4) then
            break
        endif
    od

    if (sum != 24) then
        exitnow(-1)
    endif
endin

// Test perf-rate loop
instr 6
    val:k init 0
    for val in [2, 4, 6] do
        if (val == 4) then
            break
        elseif (val == 2) then
            continue
        endif
    od
endin

</CsInstruments>
<CsScore>
i1 0 1 5
i2 0 1 5
i3 0 1
i4 0 1
i5 0 1
i6 0 1
</CsScore>
</CsoundSynthesizer>
