<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; Exercise the bounded transition-event ring through two independent readers.
; The ring capacity is 10; generating 12 events without reading forces a wrap
; and makes the first two events of that burst unavailable.

event_builder@global:i = stmcreate()
stmaddnode(event_builder, "A")       ; id 0
stmaddnode(event_builder, "B")       ; id 1
stmaddedge(event_builder, "A", "B")
stmaddedge(event_builder, "B", "A")
stmaddedge(event_builder, "B", "B")
event_definition@global:i = stmcompile(event_builder)
event_graph@global:i = stminstance(event_definition)

instr 1
    cycle:k = init(0)
    cycle += 1

    if cycle == 1 then
        ; NO_REQUEST advances the clock but must not create an event.
        advance_status:k, advance_from:k, advance_to:k = stmadvance(event_graph)
    elseif cycle == 2 then
        stmnext(event_graph, "B")
        advance_status:k, advance_from:k, advance_to:k = stmadvance(event_graph)
    elseif cycle == 3 then
        ; Queue two events before either reader runs. They must be returned in
        ; sequence over this and the following reader pass.
        stmnext(event_graph, "A")
        advance_status:k, advance_from:k, advance_to:k = stmadvance(event_graph)
        stmnext(event_graph, "B")
        advance_status, advance_from, advance_to = stmadvance(event_graph)
    elseif cycle == 5 then
        ; A legal self-edge is an event even though the current ID is unchanged.
        stmnext(event_graph, "B")
        advance_status:k, advance_from:k, advance_to:k = stmadvance(event_graph)
    elseif cycle == 6 then
        ; Reset from B to the entry node A is recorded with RESET status.
        stmreset(event_graph, 1)
    elseif cycle == 7 then
        ; Produce 12 alternating transitions. The ten retained events are
        ; sequences 9..18, so readers waiting for sequence 7 report overflow
        ; and resume from sequence 9.
        index:k = 0
        while index < 12 do
            current:k = stmcurrentid(event_graph)
            if current == 0 then
                stmnext(event_graph, "B")
            else
                stmnext(event_graph, "A")
            endif
            advance_status:k, advance_from:k, advance_to:k = stmadvance(event_graph)
            index += 1
        od
    endif

    status1:k, sequence1:k, overflow1:k, available1:k, from1:k, to1:k = stmevent(event_graph)
    status2:k, sequence2:k, overflow2:k, available2:k, from2:k, to2:k = stmevent(event_graph)

    ; Both opcode instances must observe the same stream independently.
    if available1 != available2 || overflow1 != overflow2 then
        printks("[FAIL] stmevent readers disagree: available=%f/%f overflow=%f/%f\n", 0,
                available1, available2, overflow1, overflow2)
        exitnowk(-1)
    endif
    if available1 == 1 && (status1 != status2 || sequence1 != sequence2 ||
                           from1 != from2 || to1 != to2) then
        printks("[FAIL] stmevent readers returned different events\n", 0)
        exitnowk(-1)
    endif

    if cycle == 1 && (available1 != 0 || overflow1 != 0) then
        printks("[FAIL] stmevent empty: available=%f overflow=%f\n", 0,
                available1, overflow1)
        exitnowk(-1)
    elseif cycle == 2 && (available1 != 1 || overflow1 != 0 || status1 != 1 ||
                          sequence1 != 2 || from1 != 0 || to1 != 1) then
        printks("[FAIL] stmevent changed: status/seq/overflow/available/from/to=%f/%f/%f/%f/%f/%f\n", 0,
                status1, sequence1, overflow1, available1, from1, to1)
        exitnowk(-1)
    elseif cycle == 3 && (available1 != 1 || overflow1 != 0 || status1 != 1 ||
                          sequence1 != 3 || from1 != 1 || to1 != 0) then
        printks("[FAIL] stmevent queued first: status/seq/from/to=%f/%f/%f/%f\n", 0,
                status1, sequence1, from1, to1)
        exitnowk(-1)
    elseif cycle == 4 && (available1 != 1 || overflow1 != 0 || status1 != 1 ||
                          sequence1 != 4 || from1 != 0 || to1 != 1) then
        printks("[FAIL] stmevent queued second: status/seq/from/to=%f/%f/%f/%f\n", 0,
                status1, sequence1, from1, to1)
        exitnowk(-1)
    elseif cycle == 5 && (available1 != 1 || overflow1 != 0 || status1 != 2 ||
                          sequence1 != 5 || from1 != 1 || to1 != 1) then
        printks("[FAIL] stmevent self: status/seq/from/to=%f/%f/%f/%f\n", 0,
                status1, sequence1, from1, to1)
        exitnowk(-1)
    elseif cycle == 6 && (available1 != 1 || overflow1 != 0 || status1 != 3 ||
                          sequence1 != 6 || from1 != 1 || to1 != 0) then
        printks("[FAIL] stmevent reset: status/seq/from/to=%f/%f/%f/%f\n", 0,
                status1, sequence1, from1, to1)
        exitnowk(-1)
    elseif cycle == 7 && (available1 != 1 || overflow1 != 1 || status1 != 1 ||
                          sequence1 != 9 || from1 != 0 || to1 != 1) then
        printks("[FAIL] stmevent overflow: status/seq/overflow/from/to=%f/%f/%f/%f/%f\n", 0,
                status1, sequence1, overflow1, from1, to1)
        exitnowk(-1)
    elseif cycle == 8 then
        if available1 != 1 || overflow1 != 0 || status1 != 1 ||
           sequence1 != 10 || from1 != 1 || to1 != 0 then
            printks("[FAIL] stmevent after overflow: status/seq/overflow/from/to=%f/%f/%f/%f/%f\n", 0,
                    status1, sequence1, overflow1, from1, to1)
            exitnowk(-1)
        endif
        printks("[PASS] stm event ring\n", 0)
        turnoff
    endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
