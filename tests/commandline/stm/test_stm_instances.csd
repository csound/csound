<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; One immutable definition can back multiple independent mutable runners.
shared_builder@global:i = stmcreate()
stmaddnode(shared_builder, "A")
stmaddnode(shared_builder, "B")
stmaddedge(shared_builder, "A", "B")
stmaddedge(shared_builder, "B", "A")
shared_definition@global:i = stmcompile(shared_builder)
runner1@global:i = stminstance(shared_definition)
runner2@global:i = stminstance(shared_definition)

instr 1
    cycle:k = init(0)
    cycle += 1

    current1:k = stmcurrentid(runner1)
    current2:k = stmcurrentid(runner2)
    tick1:k = stmtick(runner1)
    tick2:k = stmtick(runner2)

    if cycle == 1 then
        if current1 != 0 || current2 != 0 || tick1 != 0 || tick2 != 0 then
            printks("[FAIL] instances initial state: current=%f/%f tick=%f/%f\n", 0, current1, current2, tick1, tick2)
            exitnowk(-1)
        endif
        stmnext(runner1, "B")
        status:k, from_id:k, to_id:k = stmadvance(runner1)
    elseif cycle == 2 then
        if current1 != 1 || current2 != 0 || tick1 != 1 || tick2 != 0 then
            printks("[FAIL] runner1 affected runner2: current=%f/%f tick=%f/%f\n", 0, current1, current2, tick1, tick2)
            exitnowk(-1)
        endif
        stmnext(runner2, "B")
        status:k, from_id:k, to_id:k = stmadvance(runner2)
    elseif cycle == 3 then
        if current1 != 1 || current2 != 1 || tick1 != 1 || tick2 != 1 then
            printks("[FAIL] independent advances: current=%f/%f tick=%f/%f\n", 0, current1, current2, tick1, tick2)
            exitnowk(-1)
        endif
        stmreset(runner1, 1)
    elseif cycle == 4 then
        if current1 != 0 || current2 != 1 || tick1 != 0 || tick2 != 1 then
            printks("[FAIL] runner1 reset affected runner2: current=%f/%f tick=%f/%f\n", 0, current1, current2, tick1, tick2)
            exitnowk(-1)
        endif
        printks("[PASS] independent stm runners\n", 0)
        turnoff
    endif
endin

; A runner retains its definition after the stmcompile owner has ended.
late_definition@global:i = init(0)

instr 10
    builder:i = stmcreate()
    stmaddnode(builder, "A")
    stmaddnode(builder, "B")
    stmaddedge(builder, "A", "B")
    definition:i = stmcompile(builder)
    late_definition = definition
endin

instr 11
    runner:i = stminstance(late_definition)
    cycle:k = init(0)
    cycle += 1

    if cycle == 1 && stmcurrentid(runner) != 0 then
        printks("[FAIL] retained definition initial state\n", 0)
        exitnowk(-1)
    elseif cycle == 50 then
        ; instr 10, which owns the public definition handle, ended earlier.
        ; The runner's retained reference must still keep the topology alive.
        stmnext(runner, "B")
        status:k, from_id:k, to_id:k = stmadvance(runner)
        if status != 1 || stmcurrentid(runner) != 1 then
            printks("[FAIL] runner lost definition after definition owner ended\n", 0)
            exitnowk(-1)
        endif
        printks("[PASS] runner retained immutable definition\n", 0)
        turnoff
    endif
endin

; ---- instance overload ----
; One burst creates many runners of the shared definition in a single control
; cycle (this grows the registry well past its initial capacity), then every
; runner is driven by its own writer instance for 200 cycles with fully
; deterministic assertions. The wall-clock report shows how far the whole
; overload stays from the real-time budget of its score span.

stress_count@global:i = 300
stress_done@global:k = init(0)
stress_start@global:i = init(0)

instr 100 ; spawn the overload burst
    stress_start = rtclock()
    idx:i = 0
    while idx < stress_count do
        schedule(101, 0, 0.15)
        idx += 1
    od
endin

instr 101 ; one runner + its writer, created per spawned instance
    runner:i = stminstance(shared_definition)
    cycle:k = init(0)
    cycle += 1

    ; before the advance: tick counts prior advances, current alternates A/B
    tick:k = stmtick(runner)
    current:k = stmcurrentid(runner)
    expected:k = (cycle - 1) % 2
    if tick != cycle - 1 || current != expected then
        printks("[FAIL] overload runner: cycle=%f tick=%f current=%f\n", 0, cycle, tick, current)
        exitnowk(-1)
    endif

    if current == 0 then
        stmnext(runner, 1)
    else
        stmnext(runner, 0)
    endif
    status:k, from_id:k, to_id:k = stmadvance(runner)
    if status != 1 || from_id == to_id then
        printks("[FAIL] overload advance: status/from/to=%f/%f/%f\n", 0, status, from_id, to_id)
        exitnowk(-1)
    endif

    if cycle == 200 then
        stress_done += 1
        turnoff
    endif
endin

instr 110 ; verify completion and report the real-time margin
    if stress_done != stress_count then
        printks("[FAIL] instance overload: completed=%f expected=%f\n", 0, stress_done, stress_count)
        exitnowk(-1)
    endif
    printks("[PASS] instance overload: %f runners x 200 cycles, wall %f s over 0.18 s score span\n", 0, stress_count, rtclock() - stress_start)
    turnoff
endin
</CsInstruments>
<CsScore>
i 1   0    0.02
i 10  0    0.03
i 11  0.01 0.08
i 100 0.12 0.01
i 110 0.30 0.01
e
</CsScore>
</CsoundSynthesizer>
