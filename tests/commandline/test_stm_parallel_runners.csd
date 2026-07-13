<CsoundSynthesizer>
<CsOptions>
-n -j2
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; Two writer instruments drive two independent runners of one shared
; definition on the multicore performance path. Runner state lives behind a
; per-runner published snapshot, not a shared lock, so unrelated graphs must
; not interfere: each writer asserts its own fully deterministic tick count
; and transition sequence while both mutate concurrently.

shared_builder@global:i = stmcreate()
stmaddnode(shared_builder, "A")
stmaddnode(shared_builder, "B")
stmaddedge(shared_builder, "A", "B")
stmaddedge(shared_builder, "B", "A")
shared_definition@global:i = stmcompile(shared_builder)
runner1@global:i = stminstance(shared_definition)
runner2@global:i = stminstance(shared_definition)

instr 1 ; writer for runner1: one transition every cycle
    cycle:k = init(0)
    cycle += 1

    tick:k = stmtick(runner1)
    if tick != cycle - 1 then
        printks("[FAIL] parallel writer 1 tick=%f expected %f\n", 0, tick, cycle - 1)
        exitnowk(-1)
    endif

    current:k = stmcurrentid(runner1)
    if current == 0 then
        stmnext(runner1, 1)
    else
        stmnext(runner1, 0)
    endif
    status:k, from_id:k, to_id:k = stmadvance(runner1)
    if status != 1 || from_id == to_id then
        printks("[FAIL] parallel writer 1 status/from/to=%f/%f/%f\n", 0,
                status, from_id, to_id)
        exitnowk(-1)
    endif

    if cycle == 120 then
        printks("[PASS] parallel writer 1\n", 0)
        turnoff
    endif
endin

instr 2 ; writer for runner2: one transition every second cycle
    cycle:k = init(0)
    advances:k = init(0)
    cycle += 1

    tick:k = stmtick(runner2)
    if tick != advances then
        printks("[FAIL] parallel writer 2 tick=%f expected %f\n", 0, tick, advances)
        exitnowk(-1)
    endif

    if cycle % 2 == 0 then
        current:k = stmcurrentid(runner2)
        if current == 0 then
            stmnext(runner2, 1)
        else
            stmnext(runner2, 0)
        endif
        status:k, from_id:k, to_id:k = stmadvance(runner2)
        advances += 1
        if status != 1 || from_id == to_id then
            printks("[FAIL] parallel writer 2 status/from/to=%f/%f/%f\n", 0,
                    status, from_id, to_id)
            exitnowk(-1)
        endif
    endif

    if cycle == 120 then
        printks("[PASS] parallel writer 2\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.2
i 2 0 0.2
e
</CsScore>
</CsoundSynthesizer>
