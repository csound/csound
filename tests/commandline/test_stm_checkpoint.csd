<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 32000
ksmps  = 32
nchnls = 1
0dbfs  = 1

checkpoint_builder@global:i = stmcreate()
stmaddnode(checkpoint_builder, "A")
stmaddnode(checkpoint_builder, "B")
stmaddnode(checkpoint_builder, "C")
stmaddedge(checkpoint_builder, "A", "B")
stmaddedge(checkpoint_builder, "B", "C")
checkpoint_definition@global:i = stmcompile(checkpoint_builder)
checkpoint_runner@global:i = stminstance(checkpoint_definition)

instr 1
    cycle:k = init(0)
    cycle += 1

    if cycle == 1 then
        stmnext(checkpoint_runner, "B")
    elseif cycle == 2 || cycle == 5 then
        stmnext(checkpoint_runner, "C")
    endif
    advance_status:k, advance_from:k, advance_to:k = stmadvance(checkpoint_runner)

    ; Capture a non-initial state: B at tick 1. A reset or a hard-coded jump
    ; to the entry node cannot satisfy the resume assertions below.
    freeze_trigger:k = cycle == 1 ? 1 : 0
    captured:k = stmfreeze(checkpoint_runner, "at-B-tick-1", freeze_trigger)
    freeze_node:k = stmcurrentid(checkpoint_runner)
    freeze_tick:k = stmtick(checkpoint_runner)
    if freeze_trigger == 1 then
        printks("[checkpoint][FREEZE] cycle=%d name=at-B-tick-1 captured=%d node=%d tick=%d\n", 0, cycle, captured, freeze_node, freeze_tick)
    endif

    ; The low cycle between resume pulses must re-arm the same checkpoint.
    resume_trigger:k = cycle == 4 || cycle == 6 ? 1 : 0
    resumed:k = stmresume(checkpoint_runner, "at-B-tick-1", resume_trigger)

    current:k = stmcurrentid(checkpoint_runner)
    tick:k = stmtick(checkpoint_runner)
    event_status:k, event_sequence:k, event_overflow:k, event_available:k, event_from:k, event_to:k = stmevent(checkpoint_runner)

    printks("[checkpoint][STATE] cycle=%d node=%d tick=%d advance=%d\n", 0, cycle, current, tick, advance_status)
    if resume_trigger == 1 then
        printks("[checkpoint][RESUME] cycle=%d name=at-B-tick-1 restored=%d -> node=%d tick=%d\n", 0, cycle, resumed, current, tick)
    endif
    if event_available == 1 then
        printks("[checkpoint][EVENT] seq=%d status=%d from=%d to=%d overflow=%d\n", 0, event_sequence, event_status, event_from, event_to, event_overflow)
    else
        printks("[checkpoint][EVENT] none\n", 0)
    endif

    if cycle == 1 && (captured != 1 || current != 1 || tick != 1 || event_available != 1 || event_status != 1 || event_sequence != 2 || event_from != 0 || event_to != 1) then
        printks("[FAIL] freeze state/result/current/tick/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0,
                captured, current, tick, event_available, event_status,
                event_sequence, event_from, event_to)
        exitnowk(-1)
    elseif cycle == 2 && (current != 2 || tick != 2 || event_available != 1 || event_status != 1 || event_sequence != 3 || event_from != 1 || event_to != 2) then
        printks("[FAIL] checkpoint setup current/event=%f/%f/%f/%f\n", 0, current, event_available, event_status, event_sequence)
        exitnowk(-1)
    elseif cycle == 4 && (resumed != 1 || current != 1 || tick != 1 || event_available != 1 || event_status != 4 || event_sequence != 4 || event_from != 2 || event_to != 1) then
        printks("[FAIL] first resume result/current/tick/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0, resumed, current, tick, event_available, event_status, event_sequence, event_from, event_to)
        exitnowk(-1)
    elseif cycle == 5 && (current != 2 || tick != 2 || event_available != 1 || event_status != 1 || event_sequence != 5 || event_from != 1 || event_to != 2) then
        printks("[FAIL] post-resume transition current/event=%f/%f/%f/%f\n", 0, current, event_available, event_status, event_sequence)
        exitnowk(-1)
    elseif cycle == 6 && (resumed != 1 || current != 1 || tick != 1 || event_available != 1 || event_status != 4 || event_sequence != 6 || event_from != 2 || event_to != 1) then
        printks("[FAIL] repeated resume result/current/tick/event=%f/%f/%f/%f/%f/%f/%f/%f\n", 0, resumed, current, tick, event_available, event_status, event_sequence, event_from, event_to)
        exitnowk(-1)
    elseif cycle == 7 then
        printks("[PASS] stm checkpoint resume\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.02
e
</CsScore>
</CsoundSynthesizer>
