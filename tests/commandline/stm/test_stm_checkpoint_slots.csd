<CsTest>
description = "testing stm checkpoint slots, rising-edge triggers and same-node recall"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 32000
ksmps  = 32
nchnls = 1
0dbfs  = 1

; A checkpoint name is a save slot: saving it again overwrites it.
; stmcall and stmrecall fire on the rising edge of their trigger, like
; stmreset/stmpause/stmresume/stmdelete: any positive value fires, a held
; trigger fires once, and the done output is 1 only on the firing pass.
; A recall that lands on the current node is still an event (kind 4) but
; not an enter/exit transition. The first event of a runner has sequence 1.

slots_builder@global:i = stmcreate()
stmaddnode(slots_builder, "A")
stmaddnode(slots_builder, "B")
stmaddedge(slots_builder, "A", "B")
stmaddedge(slots_builder, "B", "A")
slots_definition@global:i = stmcompile(slots_builder)
slots_runner@global:i = stminstance(slots_definition)

instr 1
    r:i = slots_runner
    cycle:k = init(0)
    cycle += 1

    ; cycle 1: A -> B (first event, sequence 1)
    ; cycle 6: B -> A
    if cycle == 1 || cycle == 6 then
        stmnext(r, cycle == 1 ? "B" : "A")
    endif
    advance_status:k, advance_from:k, advance_to:k = stmadvance(r)

    ; save "slot" with a non-unit trigger held for cycles 2-3, then again
    ; at cycle 5 after the trigger went low: the second save overwrites
    save_trigger:k = (cycle == 2 || cycle == 3) ? 0.5 : (cycle == 5 ? 2 : 0)
    saved:k = stmcall(r, "slot", save_trigger)

    ; cycle 7 (on A): recall "slot" -> B at the tick of the cycle-5 save
    ; cycle 9 (on B): recall "slot" again -> same node B, no enter/exit
    recall_trigger:k = (cycle == 7 || cycle == 9) ? 1 : 0
    recalled:k = stmrecall(r, "slot", recall_trigger)

    current:k = stmcurrentid(r)
    tick:k = stmtick(r)
    entered_b:k = stmonenter(r, "B")
    exited_b:k = stmonexit(r, "B")
    event_status:k, event_sequence:k, event_overflow:k, event_available:k, event_from:k, event_to:k = stmevent(r)

    printks("[slots] cycle=%d node=%d tick=%d saved=%d recalled=%d enterB=%d exitB=%d event=%d/%d/%d\n", 0, cycle, current, tick, saved, recalled, entered_b, exited_b, event_available, event_status, event_sequence)

    if cycle == 1 && (current != 1 || event_available != 1 || event_status != 1 || event_sequence != 1) then
        printks("[FAIL] first event must have sequence 1\n", 0)
        exitnowk(-1)
    elseif cycle == 2 && (saved != 1 || tick != 2) then
        printks("[FAIL] a positive non-unit trigger must save\n", 0)
        exitnowk(-1)
    elseif cycle == 3 && saved != 0 then
        printks("[FAIL] a held trigger must not save again\n", 0)
        exitnowk(-1)
    elseif cycle == 5 && (saved != 1 || tick != 5) then
        printks("[FAIL] a new rising edge must save again\n", 0)
        exitnowk(-1)
    elseif cycle == 6 && current != 0 then
        printks("[FAIL] setup: expected node A\n", 0)
        exitnowk(-1)
    elseif cycle == 7 && (recalled != 1 || current != 1 || tick != 5 ||
                          entered_b != 1 || event_status != 4 || event_sequence != 3) then
        ; the overwritten slot holds B at tick 5, not the cycle-2 state
        printks("[FAIL] recall must restore the overwritten slot\n", 0)
        exitnowk(-1)
    elseif cycle == 8 && recalled != 0 then
        printks("[FAIL] recall done must be 1 only on the firing pass\n", 0)
        exitnowk(-1)
    elseif cycle == 9 && (recalled != 1 || current != 1 || tick != 5 ||
                          entered_b != 0 || exited_b != 0 ||
                          event_available != 1 || event_status != 4 || event_sequence != 4) then
        printks("[FAIL] same-node recall: event yes, enter/exit no\n", 0)
        exitnowk(-1)
    elseif cycle == 10 then
        printks("[PASS] stm checkpoint slots\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
