<CsTest>
description = "stm opcodes read their inputs before writing an output that shares the variable"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; An output may share a variable with an input. stmcompile and stminstance
; clear their output before the lookup, and stmcall/stmrecall clear their
; done output before testing the trigger: each must read the input first.

instr 1
    ; one variable walks builder -> definition -> runner
    handle:i = stmcreate()
    stmaddnode(handle, "A")
    stmaddnode(handle, "B")
    stmaddedge(handle, "A", "B")
    stmaddedge(handle, "B", "A")
    handle = stmcompile(handle)
    handle = stminstance(handle)
    r:i = handle

    cycle:k = init(0)
    cycle += 1

    ; cycle 1: a trigger set only at init fires, the init pass must not
    ; clear it through the shared output; it then holds 0 (done is 1 once)
    held:k = init(1)
    held = stmcall(r, "held", held)
    held_done:k = held

    ; cycle 2: save "slot" on A; cycle 3: A -> B
    if cycle == 3 then
        stmnext(r, "B")
    endif
    advance_status:k, advance_from:k, advance_to:k = stmadvance(r)

    save:k = cycle == 2 ? 1 : 0
    save = stmcall(r, "slot", save)

    ; cycle 5: recall "slot" -> back on A
    recall:k = cycle == 5 ? 1 : 0
    recall = stmrecall(r, "slot", recall)

    current:k = stmcurrentid(r)
    count:k = stmnodecount(r)

    printks("[alias] cycle=%d node=%d held=%d save=%d recall=%d\n", 0, cycle, current, held_done, save, recall)

    if cycle == 1 && (count != 2 || held_done != 1) then
        printks("[FAIL] init-only trigger sharing the output must fire\n", 0)
        exitnowk(-1)
    elseif cycle == 2 && (held_done != 0 || save != 1) then
        printks("[FAIL] stmcall must read a trigger that shares its output\n", 0)
        exitnowk(-1)
    elseif cycle == 4 && current != 1 then
        printks("[FAIL] setup: expected node B\n", 0)
        exitnowk(-1)
    elseif cycle == 5 && (recall != 1 || current != 0) then
        printks("[FAIL] stmrecall must read a trigger that shares its output\n", 0)
        exitnowk(-1)
    elseif cycle == 6 then
        printks("[PASS] stm output alias\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
