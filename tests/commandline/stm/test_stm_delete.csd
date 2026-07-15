<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 32000
ksmps  = 32
nchnls = 1
0dbfs  = 1

delete_builder@global:i = stmcreate()
stmaddnode(delete_builder, "A")
stmaddnode(delete_builder, "B")
stmaddedge(delete_builder, "A", "B")
stmaddedge(delete_builder, "B", "A")
delete_definition@global:i = stmcompile(delete_builder)
delete_runner@global:i = stminstance(delete_definition)

instr 1
    cycle:k = init(0)
    cycle += 1

    ; Hold the first delete trigger high, then re-arm it and request delete
    ; again. Only the first positive edge may publish an event.
    delete_trigger:k = cycle == 2 || cycle == 3 || cycle == 5 ? 1 : 0
    stmdelete(delete_runner, delete_trigger)

    ; Establish B at tick 1 before deletion, then keep trying to mutate the
    ; terminal runner through every control path.
    if cycle == 1 then
        stmnext(delete_runner, "B")
    elseif cycle >= 2 then
        stmnext(delete_runner, "A")
    endif

    mutate_trigger:k = cycle == 2 || cycle == 5 ? 1 : 0
    stmreset(delete_runner, mutate_trigger)
    stmpause(delete_runner, mutate_trigger)
    stmresume(delete_runner, mutate_trigger)

    advance_status:k, advance_from:k, advance_to:k = stmadvance(delete_runner)

    capture_trigger:k = cycle == 1 ? 1 : 0
    captured:k = stmcall(delete_runner, "before-delete", capture_trigger)
    post_capture:k = stmcall(delete_runner, "after-delete", mutate_trigger)
    recalled:k = stmrecall(delete_runner, "before-delete", mutate_trigger)

    current:k = stmcurrentid(delete_runner)
    tick:k = stmtick(delete_runner)
    event_status:k,    \
    event_sequence:k,  \
    event_overflow:k,  \
    event_available:k, \
    event_from:k,      \
    event_to:k = stmevent(delete_runner)

    if cycle == 1 then
        if advance_status != 1 || advance_from != 0 || advance_to != 1 ||
           current != 1 || tick != 1 || captured != 1 ||
           event_available != 1 || event_overflow != 0 ||
           event_sequence != 2 || event_status != 1 ||
           event_from != 0 || event_to != 1 then
            printks("[FAIL] delete setup status/from/to/current/tick/captured/event=%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                    advance_status, advance_from, advance_to, current, tick,
                    captured, event_available, event_overflow, event_sequence,
                    event_status, event_from, event_to)
            exitnowk(-1)
        endif
    elseif cycle == 2 then
        if advance_status != 6 || advance_from != 1 || advance_to != -1 ||
           current != 1 || tick != 1 || post_capture != 0 || recalled != 0 ||
           event_available != 1 || event_overflow != 0 ||
           event_sequence != 3 || event_status != 7 ||
           event_from != 1 || event_to != 1 then
            printks("[FAIL] delete terminal state/status/event=%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                    advance_status, advance_from, advance_to, current, tick,
                    post_capture, recalled, event_available, event_overflow,
                    event_sequence, event_status, event_from, event_to)
            exitnowk(-1)
        endif
    elseif cycle >= 3 then
        if advance_status != 6 || advance_from != 1 || advance_to != -1 ||
           current != 1 || tick != 1 || post_capture != 0 || recalled != 0 ||
           event_available != 0 then
            printks("[FAIL] deleted runner mutated at cycle %f: status/from/to/current/tick/call/recall/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                    cycle, advance_status, advance_from, advance_to, current,
                    tick, post_capture, recalled, event_available)
            exitnowk(-1)
        endif

        if cycle == 5 then
            printks("[PASS] stm delete is terminal and idempotent\n", 0)
            turnoff
        endif
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.02
e
</CsScore>
</CsoundSynthesizer>
