<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; ============================================================
; STM runner lifetime: retained observers and stale handles.
;
; Builder, definition-handle and runner ownership belong to their producing
; opcode instances. Runtime opcodes retain the runner during init and release
; it during deinit, so an already initialized observer may safely outlive the
; stminstance owner. Removing the registry handle prevents new users from
; joining; the runner storage is freed only after the last retained opcode ends.
;
; Cross-instrument observation is allowed without registry lookups on each
; performance pass. Older versions cached a raw GRAPH* without retaining it,
; which made this orchestra read freed memory (and segfault under an allocator
; that unmaps freed pages).
;
; EXPECTED: instr 20 remains safe after the owner ends, then instr 30 gets a
; clean init error when it tries to acquire the stale handle. The test.py entry
; requires both diagnostics and rejects common crash/sanitizer messages.
; ============================================================

handle@global:i = init(0)

instr 10 ; owner: removes the public handle when this instance ends
    builder:i = stmcreate()
    stmaddnode(builder, "A")
    stmaddnode(builder, "B")
    stmaddedge(builder, "A", "B")
    definition:i = stmcompile(builder)
    runner:i = stminstance(definition)
    handle = runner
    prints("[lifetime] instr 10: runner %d created; handle removed when owner ends\n", runner)
endin

instr 20 ; observer: outlives the owner
    trig:k = stmonenter(handle, "A")

    c:k = init(0)
    c = c + 1
    if c == 1 then
        ; while the owner is alive, the handle resolves. The initial entry is
        ; not a new graph event for this late observer, so trig is 0.
        if trig != 0 then
            printks("[FAIL] lifetime: expected on_enter=0 for late observer\n", 0)
            exitnowk(-1)
        endif
        printks("[lifetime] instr 20: graph observed while alive, trig=0\n", 0)
    endif
    if c == 70 then
        printks("[lifetime] retained observer remained safe after owner ended\n", 0)
    endif
endin

instr 30 ; a new observer cannot acquire the stale registry handle
    trig:k = stmonenter(handle, "A")
endin

</CsInstruments>
<CsScore>
i 10 0    0.05   ; owner removes registry handle at 0.05
i 20 0.01 0.08   ; retained observer runs past owner deinit
i 30 0.10 0.01   ; new observer: controlled init error on stale handle
e
</CsScore>
</CsoundSynthesizer>
