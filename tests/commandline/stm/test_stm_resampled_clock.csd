<CsTest>
description = "stm clocks use engine-rate time for oversampled and undersampled readers and writers"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 32
ksmps  = 8
nchnls = 1
0dbfs  = 1

; ============================================================
; One engine control period lasts ksmps / sr = 0.25 s. The graph clock
; must report that period in seconds no matter which local rate the
; advancing opcode or the reading opcode runs at:
;
;   - oversample 2: ksmps 8 at sr 64, two passes per engine cycle,
;     each pass advancing 0.125 s
;   - undersample 2: ksmps 4 at sr 16, one pass per engine cycle,
;     advancing 0.25 s
;
; Every writer requests A -> B on its second engine cycle, so before
; the advance of cycle c the clocks read
;
;   stmtime     = (c - 1) * 0.25
;   stmnodetime = stmtime for c <= 2, then stmtime - 0.5
; ============================================================

opcode build_runner():i
    builder:i = stmcreate()
    stmaddnode(builder, "A")
    stmaddnode(builder, "B")
    stmaddedge(builder, "A", "B")
    definition:i = stmcompile(builder)
    runner:i = stminstance(definition)
    xout(runner)
endop

body_writer@global:i = build_runner()
oversampled_writer@global:i = build_runner()
undersampled_writer@global:i = build_runner()

; k outputs of a resampled UDO pass through a sample-rate converter, so
; the readers report through globals instead
read_time@global:k = 0
read_node_time@global:k = 0

opcode read_clock_oversampled(g:i):void
    oversample(2)
    read_time = stmtime(g)
    read_node_time = stmnodetime(g)
endop

opcode read_clock_undersampled(g:i):void
    undersample(2)
    read_time = stmtime(g)
    read_node_time = stmnodetime(g)
endop

; the second pass of engine cycle 2 is tick 3
opcode advance_oversampled(g:i):void
    oversample(2)
    if stmtick(g) == 3 then
        stmnext(g, "B")
    endif
    status:k, from_id:k, to_id:k = stmadvance(g)
endop

; one pass per engine cycle, so engine cycle 2 is tick 1
opcode advance_undersampled(g:i):void
    undersample(2)
    if stmtick(g) == 1 then
        stmnext(g, "B")
    endif
    status:k, from_id:k, to_id:k = stmadvance(g)
endop

opcode expect_clock(label:S, c:k, gt:k, nt:k):void
    expected_time:k = (c - 1) * 0.25
    expected_node:k = c <= 2 ? expected_time : expected_time - 0.5
    if abs:k(gt - expected_time) > 1e-6 || abs:k(nt - expected_node) > 1e-6 then
        printks("[FAIL] %s cycle %d: time=%f node=%f expected %f and %f\n", 0,
                label, c, gt, nt, expected_time, expected_node)
        exitnowk(-1)
    endif
endop

; Read the same runner from the instrument body and from both resampled
; UDOs. Nothing advances between these reads.
opcode expect_all_readers(label:S, g:i, c:k):void
    gt:k = stmtime(g)
    nt:k = stmnodetime(g)
    expect_clock(strcat(label, " body reader"), c, gt, nt)
    read_clock_oversampled(g)
    expect_clock(strcat(label, " oversampled reader"), c, read_time, read_node_time)
    read_clock_undersampled(g)
    expect_clock(strcat(label, " undersampled reader"), c, read_time, read_node_time)
endop

instr 1 ; writer in the instrument body
    c:k = init(0)
    c = c + 1

    expect_all_readers("body writer", body_writer, c)
    if c == 2 then
        stmnext(body_writer, "B")
    endif
    status:k, from_id:k, to_id:k = stmadvance(body_writer)

    if c == 5 then
        println("[DONE] STM BODY WRITER RESAMPLED READERS TEST PASSED\n")
        turnoff
    endif
endin

instr 2 ; writer inside an oversample 2 UDO: two ticks per engine cycle
    c:k = init(0)
    c = c + 1

    expect_all_readers("oversampled writer", oversampled_writer, c)
    tick:k = stmtick(oversampled_writer)
    if tick != 2 * (c - 1) then
        printks("[FAIL] oversampled writer cycle %d: tick=%d expected %d\n", 0,
                c, tick, 2 * (c - 1))
        exitnowk(-1)
    endif
    advance_oversampled(oversampled_writer)

    if c == 5 then
        println("[DONE] STM OVERSAMPLED WRITER TEST PASSED\n")
        turnoff
    endif
endin

instr 3 ; writer inside an undersample 2 UDO: one tick per engine cycle
    c:k = init(0)
    c = c + 1

    expect_all_readers("undersampled writer", undersampled_writer, c)
    tick:k = stmtick(undersampled_writer)
    if tick != c - 1 then
        printks("[FAIL] undersampled writer cycle %d: tick=%d expected %d\n", 0,
                c, tick, c - 1)
        exitnowk(-1)
    endif
    advance_undersampled(undersampled_writer)

    if c == 5 then
        println("[DONE] STM UNDERSAMPLED WRITER TEST PASSED\n")
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 10
i 2 0 10
i 3 0 10
</CsScore>
</CsoundSynthesizer>
