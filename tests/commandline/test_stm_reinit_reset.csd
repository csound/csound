<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; One regression test for two lifetime/clock cases:
;   1. reinit must release the graph previously owned by stmcreate
;   2. stmreset suppresses an advance only in the reset performance cycle

reset_builder@global:i = stmcreate()
stmaddnode(reset_builder, "A")
reset_definition@global:i = stmcompile(reset_builder)
reset_graph@global:i = stminstance(reset_definition)

old_handle@global:i = init(0)
new_handle@global:i = init(0)
create_count@global:i = init(0)

instr 1
CREATE:
    h:i = stmcreate()
    create_count += 1
    if create_count == 1 then
        old_handle = h
    else
        new_handle = h
    endif
    prints("[reinit-reset] stmcreate handle %d\n", h)
    rireturn

    reined:k = init(0)
    if reined == 0 && timeinsts() >= 0.02 then
        reined = 1
        reinit CREATE
    endif
endin

instr 2
    ; A released slot is reused with its next generation. With the documented
    ; 4096-slot handle encoding, the replacement is old_handle + 4096. Before
    ; the fix the old graph remained registered and this was a different slot.
    if create_count != 2 || new_handle != old_handle + 4096 then
        prints("[FAIL] reinit cleanup: old=%d new=%d creates=%d\n", old_handle, new_handle, create_count)
        exitnow(-1)
    endif
    prints("[PASS] reinit cleanup: old=%d new=%d\n", old_handle, new_handle)
endin

instr 10
    ; An advance in the reset cycle must leave the restarted clock at zero.
    stmreset(reset_graph)
    status:k, from_id:k, to_id:k = stmadvance(reset_graph)
    turnoff
endin

instr 11
    tick:k = stmtick(reset_graph)
    if tick != 0 then
        printks("[FAIL] same-cycle reset: tick=%f expected 0\n", 0, tick)
        exitnowk(-1)
    endif

    ; This reset is deliberately not followed by an advance in this cycle.
    stmreset(reset_graph)
    turnoff
endin

instr 12
    ; This is a genuinely later performance cycle and must not be suppressed.
    status:k, from_id:k, to_id:k = stmadvance(reset_graph)
    turnoff
endin

instr 13
    tick:k = stmtick(reset_graph)
    if tick != 1 then
        printks("[FAIL] delayed advance: tick=%f expected 1\n", 0, tick)
        exitnowk(-1)
    endif
    printks("[PASS] reset cycle tracking\n", 0)
    turnoff
endin
</CsInstruments>
<CsScore>
i 1  0    0.08
i 2  0.05 0.01
i 10 0.10 0.01
i 11 0.15 0.01
i 12 0.20 0.01
i 13 0.25 0.01
e
</CsScore>
</CsoundSynthesizer>
