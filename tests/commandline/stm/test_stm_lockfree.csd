<CsoundSynthesizer>
<CsOptions>
-n -j2
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; One writer continuously publishes transitions while two independent
; observers read current state, clock values and the event ring on Csound's
; multicore performance path. Readers may run before or after the writer in a
; cycle, so assertions cover snapshot validity and monotonicity rather than an
; incidental scheduler order.

builder@global:i = stmcreate()
stmaddnode(builder, "A")
stmaddnode(builder, "B")
stmaddedge(builder, "A", "B")
stmaddedge(builder, "B", "A")
definition@global:i = stmcompile(builder)
runner@global:i = stminstance(definition)

instr 1 ; sole writer
    current:k = stmcurrentid(runner)
    if current == 0 then
        stmnext(runner, 1)
    else
        stmnext(runner, 0)
    endif
    status:k, from_id:k, to_id:k = stmadvance(runner)
    if status != 1 || from_id == to_id then
        printks("[FAIL] lock-free writer status/from/to=%f/%f/%f\n", 0,
                status, from_id, to_id)
        exitnowk(-1)
    endif
endin

instr 2 ; lock-free observer, instantiated twice
    current:k = stmcurrentid(runner)
    tick:k = stmtick(runner)
    graph_time:k = stmtime(runner)
    node_time:k = stmnodetime(runner)
    status:k, sequence:k, overflow:k, available:k, from_id:k, to_id:k = stmevent(runner)

    previous_tick:k = init(0)
    previous_time:k = init(0)
    previous_sequence:k = init(0)
    cycle:k = init(0)
    cycle += 1

    if current < 0 || current > 1 || tick < previous_tick ||
       graph_time < previous_time || node_time < 0 then
        printks("[FAIL] lock-free snapshot current/tick/prev/time/node=%f/%f/%f/%f/%f\n", 0,
                current, tick, previous_tick, graph_time, node_time)
        exitnowk(-1)
    endif

    if available == 1 then
        if status != 1 || sequence <= previous_sequence ||
           from_id < 0 || from_id > 1 || to_id < 0 || to_id > 1 ||
           from_id == to_id then
            printks("[FAIL] lock-free event status/seq/prev/from/to=%f/%f/%f/%f/%f\n", 0,
                    status, sequence, previous_sequence, from_id, to_id)
            exitnowk(-1)
        endif
        previous_sequence = sequence
    endif
    previous_tick = tick
    previous_time = graph_time

    if cycle == 150 then
        printks("[PASS] lock-free observer %d\n", 0, p4)
        turnoff
    endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.20
i 2 0 0.12 1
i 2 0 0.12 2
e
</CsScore>
</CsoundSynthesizer>
