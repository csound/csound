<CsTest>
description = "stm owner deinit keeps a newer handle written to a shared output by an overlapping owner"

[expect]
exit = 0
output_excludes = ["[FAIL]", "invalid runner", "invalid definition", "Segmentation fault", "AddressSanitizer", "UndefinedBehaviorSanitizer", "Abort trap"]
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

; Two overlapping owners write their handles to the same globals. When the
; older owner ends, its deinit must release its own objects without clearing
; the newer owner's handles; once the newer owner ends, the globals clear.

shared_builder@global:i = init(0)
shared_definition@global:i = init(0)
shared_runner@global:i = init(0)
newest_builder@global:i = init(0)
newest_definition@global:i = init(0)
newest_runner@global:i = init(0)

instr 1
    shared_builder = stmcreate()
    stmaddnode(shared_builder, "A")
    stmaddnode(shared_builder, "B")
    shared_definition = stmcompile(shared_builder)
    shared_runner = stminstance(shared_definition)
    newest_builder = shared_builder
    newest_definition = shared_definition
    newest_runner = shared_runner
    prints("[shared] owner %g: builder=%d definition=%d runner=%d\n", p1, shared_builder, shared_definition, shared_runner)
endin

; older owner (1.1) has ended, newer owner (1.2) is still alive
instr 2
    prints("[shared] after older owner: builder=%d definition=%d runner=%d\n", shared_builder, shared_definition, shared_runner)
    if shared_builder == 0 || shared_builder != newest_builder then
        prints("[FAIL] older stmcreate owner cleared the newer builder handle\n")
        exitnow(-1)
    elseif shared_definition == 0 || shared_definition != newest_definition then
        prints("[FAIL] older stmcompile owner cleared the newer definition handle\n")
        exitnow(-1)
    elseif shared_runner == 0 || shared_runner != newest_runner then
        prints("[FAIL] older stminstance owner cleared the newer runner handle\n")
        exitnow(-1)
    endif

    ; the surviving handles must still resolve
    local_runner:i = stminstance(shared_definition)
    count:k = stmnodecount(shared_runner)
    local_count:k = stmnodecount(local_runner)
    if count != 2 || local_count != 2 then
        printks("[FAIL] surviving handles do not resolve: count=%d local=%d\n", 0, count, local_count)
        exitnowk(-1)
    endif
    turnoff
endin

; both owners have ended: each cleared the output it still owned
instr 3
    if shared_builder != 0 || shared_definition != 0 || shared_runner != 0 then
        prints("[FAIL] last owner left its handles: builder=%d definition=%d runner=%d\n", shared_builder, shared_definition, shared_runner)
        exitnow(-1)
    endif
    prints("[PASS] stm shared owner output\n")
endin

</CsInstruments>
<CsScore>
i 1.1 0    0.05
i 1.2 0.02 0.20
i 2   0.10 0.01
i 3   0.30 0.01
e
</CsScore>
</CsoundSynthesizer>
