<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 32000
ksmps  = 32
nchnls = 1
0dbfs  = 1

pause_builder@global:i = stmcreate()
stmaddnode(pause_builder, "A")
stmaddnode(pause_builder, "B")
stmaddedge(pause_builder, "A", "B")
pause_definition@global:i = stmcompile(pause_builder)
pause_runner@global:i = stminstance(pause_definition)

instr 1
    cycle:k = init(0)
    cycle += 1

    ; Requests made while paused must be ignored. Only the cycle-5 request is
    ; allowed to survive and move the graph from A to B.
    if cycle == 2 || cycle == 3 || cycle == 5 then
        stmnext(pause_runner, "B")
    endif

    advance_status:k, \
    advance_from:k,   \
    advance_to:k = stmadvance(pause_runner)

    ; Hold both controls high for two cycles. State changes and their events
    ; must remain idempotent rather than being emitted once per k-cycle.
    pause_trigger:k = cycle == 1 || cycle == 2 ? 1 : 0
    resume_trigger:k = cycle == 3 || cycle == 4 ? 1 : 0
    stmpause(pause_runner, pause_trigger)
    stmresume(pause_runner, resume_trigger)

    current:k = stmcurrentid(pause_runner)
    tick:k = stmtick(pause_runner)

    event_status:k,    \
    event_sequence:k,  \
    event_overflow:k,  \
    event_available:k, \
    event_from:k,      \
    event_to:k = stmevent(pause_runner)

    entered_b:k = stmonenter(pause_runner, "B")
    exited_a:k = stmonexit(pause_runner, "A")

    printks("[pause-resume][STATE] cycle=%d node=%d tick=%d advance=%d from=%d to=%d\n", 0, cycle, current, tick, advance_status, advance_from, advance_to)
    if pause_trigger == 1 then
        printks("[pause-resume][PAUSE] cycle=%d trigger=1\n", 0, cycle)
    endif
    if resume_trigger == 1 then
        printks("[pause-resume][RESUME] cycle=%d trigger=1\n", 0, cycle)
    endif
    if event_available == 1 then
        printks("[pause-resume][EVENT] seq=%d status=%d from=%d to=%d overflow=%d\n", 0, event_sequence, event_status, event_from, event_to, event_overflow)
    else
        printks("[pause-resume][EVENT] none\n", 0)
    endif
    printks("[pause-resume][ENTER-EXIT] enter-B=%d exit-A=%d\n", 0, entered_b, exited_a)

    if cycle == 1 &&
       (current != 0 || tick != 1 || advance_status != 0 || advance_from != 0 || advance_to != -1 ||
        event_available != 1 || event_overflow != 0 || event_sequence != 2 ||
        event_status != 5 || event_from != 0 || event_to != 0 || entered_b != 0 || exited_a != 0) then
        printks("[FAIL] initial pause state/event=%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                current, tick, advance_status, advance_from, advance_to,
                event_available, event_sequence, event_status, event_from, event_to,
                entered_b, exited_a)
        exitnowk(-1)
    elseif cycle == 2 &&
           (current != 0 || tick != 1 || advance_status != 5 || advance_from != 0 || advance_to != -1 || event_available != 0 || entered_b != 0 || exited_a != 0) then
        printks("[FAIL] held pause state/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0, current, tick, advance_status, advance_from, advance_to, event_available, entered_b, exited_a)
        exitnowk(-1)
    elseif cycle == 3 &&
           (current != 0 || tick != 1 || advance_status != 5 || advance_from != 0 || advance_to != -1 ||
            event_available != 1 || event_overflow != 0 || event_sequence != 3 ||
            event_status != 6 || event_from != 0 || event_to != 0 || entered_b != 0 || exited_a != 0) then
        printks("[FAIL] resume state/event=%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                current, tick, advance_status, advance_from, advance_to,
                event_available, event_sequence, event_status, event_from, event_to,
                entered_b, exited_a)
        exitnowk(-1)
    elseif cycle == 4 &&
           (current != 0 || tick != 2 || advance_status != 0 || advance_from != 0 || advance_to != -1 ||
            event_available != 0 || entered_b != 0 || exited_a != 0) then
        printks("[FAIL] post-resume idle state/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                current, tick, advance_status, advance_from, advance_to,
                event_available, entered_b, exited_a)
        exitnowk(-1)
    elseif cycle == 5 &&
           (current != 1 || tick != 3 || advance_status != 1 || advance_from != 0 || advance_to != 1 ||
            event_available != 1 || event_overflow != 0 || event_sequence != 4 ||
            event_status != 1 || event_from != 0 || event_to != 1 || entered_b != 1 || exited_a != 1) then
        printks("[FAIL] post-resume transition state/event=%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                current, tick, advance_status, advance_from, advance_to,
                event_available, event_sequence, event_status, event_from, event_to,
                entered_b, exited_a)
        exitnowk(-1)
    elseif cycle == 6 &&
           (current != 1 || tick != 4 || advance_status != 0 || advance_from != 1 || advance_to != -1 ||
            event_available != 0 || entered_b != 0 || exited_a != 0) then
        printks("[FAIL] final state/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                current, tick, advance_status, advance_from, advance_to,
                event_available, entered_b, exited_a)
        exitnowk(-1)
    elseif cycle == 6 then
        printks("[PASS] stm pause and resume\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.02
e
</CsScore>
</CsoundSynthesizer>
