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
; STM graph lifetime: observing a graph that has been freed.
;
; A graph belongs to the instrument instance that called stmcreate:
; its deinit frees the GRAPH. An instrument that outlives the owner
; must NOT be able to dereference it.
;
; stmonenter/stmonexit resolve the handle on every perf pass exactly
; for this reason. They used to cache the GRAPH* at init, which made
; this orchestra read freed memory (and segfault under an allocator
; that unmaps freed pages).
;
; EXPECTED: a clean perf error, not a crash. This csd is registered
; in test.py as an expected failure (non-zero return code).
;
; Note: the return code alone cannot tell a clean error apart from a
; crash. The real regression guard is running this file under a
; sanitizer (-fsanitize=address), where the old code aborts.
; ============================================================

handle@global:i = init(0)

instr 10 ; owner: the graph dies with this instance
    h:i = stmcreate()
    stmaddnode(h, "A")
    stmaddnode(h, "B")
    stmaddedge(h, "A", "B")
    stmcompile(h)
    handle = h
    prints("[lifetime] instr 10: graph %d created, freed when this instance ends\n", h)
endin

instr 20 ; observer: outlives the owner
    trig:k = stmonenter(handle, "A")

    c:k = init(0)
    c = c + 1
    if c == 1 then
        ; while the owner is alive, the entry node reads as just entered
        if trig != 1 then
            printks("[FAIL] lifetime: expected on_enter=1 on the first cycle\n", 0)
            exitnowk(-1)
        endif
        printks("[lifetime] instr 20: graph observed while alive, trig=1\n", 0)
    endif
endin

</CsInstruments>
<CsScore>
i 10 0    0.05   ; owner: graph freed at 0.05
i 20 0.01 0.40   ; observer: still running well past 0.05
e
</CsScore>
</CsoundSynthesizer>
