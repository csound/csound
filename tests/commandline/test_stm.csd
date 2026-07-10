<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

#include "libassert.orc"

seed 1   ; deterministic random drive for Example 2

; ============================================================
; STM graph runtime test (imperative nodes + conditional edge)
;
; The graph owns the state machine (nodes + edges). Node
; computation is an ordinary UDO that takes the shared struct by
; reference and calls stmnext to request a transition. Each
; k-cycle the orchestra asks for the current node (stmcurrent),
; dispatches to the matching UDO, then applies the pending
; transition (stmadvance).
;
; stmaddcondedge declares the SET of targets a node may branch to
; (a conditional edge). Freeze here branches to Texture OR Analyze;
; the node picks which with stmnext, validated against that set.
;
; State  = shared struct (AudioState)
; Node   = ordinary UDO(g:i, st:AudioState)
; Graph  = Analyze -> Texture -> Freeze -> {Texture | Analyze}
; ============================================================

struct AudioState rms:k, gesture:k, density:k, grainSize:k, node:k

; ---- node computations (call stmnext to request a transition) ----
opcode AnalyzeNode(g:i, st:AudioState):void
    st.node = 1
    if st.rms > 0.2 then
        stmnext(g, "Texture")
    endif
endop

opcode TextureNode(g:i, st:AudioState):void
    st.node = 2
    st.density = 40
    st.grainSize = 0.08
    if st.gesture == 1 then
        stmnext(g, "Freeze")
    endif
endop

; Freeze is a CONDITIONAL node: it branches to Texture OR Analyze.
opcode FreezeNode(g:i, st:AudioState):void
    st.node = 3
    st.density = 1
    if st.rms < 0.05 then
        stmnext(g, "Analyze")
    else
        stmnext(g, "Texture")
    endif
endop

; ---- dispatch: run the current node by name ----
opcode STM_DISPATCH(g:i, st:AudioState, cur:S):void
    if strcmpk(cur, "Analyze") == 0 then
        AnalyzeNode(g, st)
    elseif strcmpk(cur, "Texture") == 0 then
        TextureNode(g, st)
    elseif strcmpk(cur, "Freeze") == 0 then
        FreezeNode(g, st)
    endif
endop

; ---- graph build ----
graph@global:i = stmcreate()
stmaddnode(graph, "Analyze")
stmaddnode(graph, "Texture")
stmaddnode(graph, "Freeze")
stmaddedge(graph, "Analyze", "Texture")
stmaddedge(graph, "Texture", "Freeze")

; conditional branch: Freeze -> one of {Texture, Analyze}
stmaddcondedge(graph, "Freeze", ["Texture", "Analyze"])

stmcompile(graph)

; ---- Example 1: deterministic drive + per-cycle assertions ----
state@global:AudioState = init(0, 0, 0, 0.05, 0)

instr 1
    c:k = timeinstk()

    if c == 1 then
        state.rms = 0.3          ; Analyze -> Texture
        state.gesture = 0
    elseif c == 2 then
        state.rms = 0.1          ; Texture, gesture=1 -> Freeze
        state.gesture = 1
    elseif c == 3 then
        state.rms = 0.01         ; Freeze, rms<0.05 -> Analyze (conditional)
        state.gesture = 0
    elseif c == 4 then
        state.rms = 0.01         ; back at Analyze, rms<0.2 -> stay
        state.gesture = 0
    endif
    ; c == 5: no branch, state persists (rms=0.01) -> Analyze stays

    cur:S = stmcurrent(graph)

    ; on_enter monitors, read before advance: each fires 1 only for a real
    ; graph event observed after this opcode instance was initialized.
    enter_a:k = stmonenter(graph, "Analyze")
    enter_t:k = stmonenter(graph, "Texture")
    enter_f:k = stmonenter(graph, "Freeze")

    ; on_exit monitors: each fires 1 for a real graph event where its node
    ; stopped being current.
    exit_a:k = stmonexit(graph, "Analyze")
    exit_t:k = stmonexit(graph, "Texture")
    exit_f:k = stmonexit(graph, "Freeze")

    STM_DISPATCH(graph, state, cur)
    changed:k = stmadvance(graph)

    ; ---- flow trace ----
    println("[E1] c%d  cur=%s  node=%d  enter[A/T/F]=%d/%d/%d  exit[A/T/F]=%d/%d/%d  changed=%d", c, cur, state.node, enter_a, enter_t, enter_f, exit_a, exit_t, exit_f, changed)

    if c == 1 then
        if state.node != 1 then
            printks("[FAIL] c1: node=%f expected 1 (Analyze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 1 then
            printks("[FAIL] c1: expected transition to Texture\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 || enter_t != 0 || enter_f != 0 then
            printks("[FAIL] c1: on_enter A/T/F=%f/%f/%f expected 0/0/0 (initial state is not a new event)\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        if exit_a != 0 || exit_t != 0 || exit_f != 0 then
            printks("[FAIL] c1: on_exit A/T/F=%f/%f/%f expected 0/0/0 (nothing left yet)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 2 then
        if state.node != 2 then
            printks("[FAIL] c2: node=%f expected 2 (Texture)\n", 0, state.node)
            exitnowk(-1)
        endif
        if state.density != 40 then
            printks("[FAIL] c2: density=%f expected 40 (by-ref write)\n", 0, state.density)
            exitnowk(-1)
        endif
        if changed != 1 then
            printks("[FAIL] c2: expected transition to Freeze\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 || enter_t != 1 || enter_f != 0 then
            printks("[FAIL] c2: on_enter A/T/F=%f/%f/%f expected 0/1/0\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        ; Analyze was left after c1 -> its on_exit fires now
        if exit_a != 1 || exit_t != 0 || exit_f != 0 then
            printks("[FAIL] c2: on_exit A/T/F=%f/%f/%f expected 1/0/0 (left Analyze)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 3 then
        if state.node != 3 then
            printks("[FAIL] c3: node=%f expected 3 (Freeze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 1 then
            printks("[FAIL] c3: expected conditional transition to Analyze\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 || enter_t != 0 || enter_f != 1 then
            printks("[FAIL] c3: on_enter A/T/F=%f/%f/%f expected 0/0/1\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        ; Texture was left after c2 -> its on_exit fires now
        if exit_a != 0 || exit_t != 1 || exit_f != 0 then
            printks("[FAIL] c3: on_exit A/T/F=%f/%f/%f expected 0/1/0 (left Texture)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 4 then
        if state.node != 1 then
            printks("[FAIL] c4: node=%f expected 1 (Analyze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 0 then
            printks("[FAIL] c4: unexpected transition (no request)\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 1 || enter_t != 0 || enter_f != 0 then
            printks("[FAIL] c4: on_enter A/T/F=%f/%f/%f expected 1/0/0 (re-entry Analyze)\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        ; Freeze was left after c3 (conditional -> Analyze) -> its on_exit fires now
        if exit_a != 0 || exit_t != 0 || exit_f != 1 then
            printks("[FAIL] c4: on_exit A/T/F=%f/%f/%f expected 0/0/1 (left Freeze)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 5 then
        ; Analyze stays (no request): on_enter must NOT refire.
        if state.node != 1 then
            printks("[FAIL] c5: node=%f expected 1 (Analyze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 0 then
            printks("[FAIL] c5: unexpected transition (no request)\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 then
            printks("[FAIL] c5: on_enter Analyze=%f expected 0 (no refire while staying)\n", 0, enter_a)
            exitnowk(-1)
        endif
        if exit_a != 0 || exit_t != 0 || exit_f != 0 then
            printks("[FAIL] c5: on_exit A/T/F=%f/%f/%f expected 0/0/0 (staying, none left)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
        printks("[DONE] STM TEST PASSED\n\n", 0)
        turnoff
    endif
endin

; ============================================================
; Example 2: simulated signal analysis with a random signal.
; Second graph + state (the registry hosts many graphs).
; Two instruments: capture (writes state) and test (invariants).
; ============================================================

graph2@global:i = stmcreate()
stmaddnode(graph2, "Analyze")
stmaddnode(graph2, "Texture")
stmaddnode(graph2, "Freeze")
stmaddedge(graph2, "Analyze", "Texture")
stmaddedge(graph2, "Texture", "Freeze")
stmaddcondedge(graph2, "Freeze", ["Texture", "Analyze"])
stmcompile(graph2)

state2@global:AudioState = init(0, 0, 0, 0.05, 0)

; ---- Example 2: capture / analysis (writes shared state) ----
instr 10
    ; simulated analysis: random RMS that actually crosses the graph
    ; thresholds (Analyze needs rms>0.2, Freeze branches on rms<0.05).
    lvl:k = random:k(0, 0.3)
    state2.rms = lvl

    gest:k = random:k(0, 1)            ; occasional gesture trigger
    state2.gesture = (gest > 0.7 ? 1 : 0)
endin

; ---- Example 2: test (invariants under random drive) ----
instr 11
    Scur = stmcurrent(graph2)
    STM_DISPATCH(graph2, state2, Scur)
    changed:k = stmadvance(graph2)

    node:k = state2.node

    ; a node always ran, so its id marker is always in {1,2,3}
    if node < 1 || node > 3 then
        printks("[FAIL] rnd: invalid node id %f\n", 0, node)
        exitnowk(-1)
    endif
    if changed != 0 && changed != 1 then
        printks("[FAIL] rnd: invalid changed flag %f\n", 0, changed)
        exitnowk(-1)
    endif

    ; ---- coverage tracking: prove the drive actually exercises the graph ----
    seen_a:k init 0
    seen_t:k init 0
    seen_f:k init 0
    ntrans:k init 0
    if node == 1 then
        seen_a = 1
    elseif node == 2 then
        seen_t = 1
    elseif node == 3 then
        seen_f = 1
    endif
    if changed == 1 then
        ntrans = ntrans + 1
    endif

    t:k = timeinstk()

    ; ---- flow trace: transitions + periodic heartbeat ----
    if changed == 1 then
        Snew:S = stmcurrent(graph2)
        println("[E2] t%d  transition -> %s", t, Snew)
    endif
    if t == 50 || t == 100 || t == 150 || t == 200 then
        Shb:S = stmcurrent(graph2)
        println("[E2] t%d  heartbeat  cur=%s  node=%d  rms=%.3f", t, Shb, node, state2.rms)
    endif

    if t >= 200 then
        ; coverage: the random drive must have visited every node and
        ; actually moved the graph (otherwise the test is vacuous)
        if seen_a != 1 || seen_t != 1 || seen_f != 1 then
            println("[FAIL] rnd: coverage incomplete, seen A/T/F=%d/%d/%d", seen_a, seen_t, seen_f)
            exitnowk(-1)
        endif
        if ntrans < 3 then
            println("[FAIL] rnd: too few transitions (%d) - drive not exercising graph", ntrans)
            exitnowk(-1)
        endif
        println("[E2] coverage: seen A/T/F=%d/%d/%d  transitions=%d", seen_a, seen_t, seen_f, ntrans)
        printks("[DONE] STM RANDOM TEST PASSED\n\n", 0)
        turnoff
    endif
endin

; ============================================================
; Example 3: instruments as stateful processes (convention).
;
; A graph node is bound BY CONVENTION to a Csound instrument:
;   on_enter -> spawn the instrument (schedulek), held with p3=-1
;   on_exit  -> kill it (turnoff2)
; The rising/falling-edge hooks make spawn/kill fire exactly once
; per transition (no per-cycle re-spawn). active() proves the
; process runs precisely while its node is current; each process
; accumulates its own state in a global (it is stateful).
;
; Graph3: A <-> B, each node = a long-running process instrument.
; ============================================================

graph3@global:i = stmcreate()
stmaddnode(graph3, "A")
stmaddnode(graph3, "B")
stmaddedge(graph3, "A", "B")
stmaddedge(graph3, "B", "A")
stmcompile(graph3)

; per-process accumulators (shared state the processes own)
hits_a@global:k = init(0)
hits_b@global:k = init(0)

; ---- the two "process" instruments: long-running, stateful ----
instr 30 ; process bound to node A
    hits_a = hits_a + 1
endin
instr 31 ; process bound to node B
    hits_b = hits_b + 1
endin

; ---- supervisor: binds nodes to instruments via enter/exit ----
instr 20
    c:k = timeinstk()

    ; Initial state is not a transition event for a freshly initialized
    ; observer, so bind the entry process explicitly.
    if c == 1 then
        schedulek(30, 0, -1)
        println("[E3] c%d  INITIAL A -> spawn instr 30", c)
    endif

    ; deterministic drive: A (start) -> B @c5 -> A @c10
    if c == 5 then
        println("[INFO] GOTO [B]")
        stmnext(graph3, "B")
    elseif c == 10 then
        println("[INFO] GOTO [A]")
        stmnext(graph3, "A")
    endif

    ; --- convention: node <-> instrument process lifecycle ---
    if stmonenter(graph3, "A") == 1 then
        schedulek(30, 0, -1) ; spawn process A (held)
        println("[E3] c%d  ENTER A -> spawn instr 30", c)
    endif
    if stmonexit(graph3, "A") == 1 then
        turnoff2(30, 0, 0) ; kill process A (no release)
        println("[E3] c%d  EXIT  A -> kill  instr 30", c)
    endif
    if stmonenter(graph3, "B") == 1 then
        schedulek(31, 0, -1)
        println("[E3] c%d  ENTER B -> spawn instr 31", c)
    endif
    if stmonexit(graph3, "B") == 1 then
        turnoff2(31, 0, 0)
        println("[E3] c%d  EXIT  B -> kill  instr 31", c)
    endif

    changed:k = stmadvance(graph3)   ; transition applied (return unused here)

    ; --- invariant: exactly the current node's process is alive ---
    ; checked 2 cycles after each spawn/kill so events have settled
    na:k = active:k(30)
    nb:k = active:k(31)

    ; ---- flow trace ----
    cur3:S = stmcurrent(graph3)
    println("[E3] c%d  cur=%s  activeA=%d  activeB=%d", c, cur3, na, nb)

    if c == 3 then                    ; settled in A
        if na != 1 || nb != 0 then
            printks("[FAIL] p c3: active A/B=%f/%f expected 1/0\n", 0, na, nb)
            exitnowk(-1)
        endif
        if hits_a <= 0 then
            printks("[FAIL] p c3: process A did not run (hits=%f)\n", 0, hits_a)
            exitnowk(-1)
        endif
    elseif c == 8 then                ; settled in B
        if na != 0 || nb != 1 then
            printks("[FAIL] p c8: active A/B=%f/%f expected 0/1\n", 0, na, nb)
            exitnowk(-1)
        endif
    elseif c == 13 then               ; back in A
        if na != 1 || nb != 0 then
            printks("[FAIL] p c13: active A/B=%f/%f expected 1/0\n", 0, na, nb)
            exitnowk(-1)
        endif
        turnoff2(30, 0, 0)
        printks("[DONE] STM PROCESS TEST PASSED\n\n", 0)
        turnoff
    endif
endin

; ============================================================
; Example 4: introspection + id-based API + entry/reset.
;
; Exercises the newer opcodes:
;   stmentry     - set a non-default entry node (before compile)
;   stmnodecount - number of nodes
;   stmedgecount - total number of edges
;   stmnodeid    - name -> id
;   stmnodename  - id   -> name
;   stmcurrentid - current node as id (dispatch by index, not name)
;   stmnext(id)  - request a transition by id (resolves to the .id overload)
;   stmreset     - restore the graph to its entry node
;
; Graph4: Idle -> Run -> Done -> Idle (a cycle). Entry is forced to
; "Run" via stmentry, so the machine does NOT start at node 0.
; ============================================================

graph4@global:i = stmcreate()
stmaddnode(graph4, "Idle")     ; id 0
stmaddnode(graph4, "Run")      ; id 1
stmaddnode(graph4, "Done")     ; id 2
stmaddedge(graph4, "Idle", "Run")
stmaddedge(graph4, "Run", "Done")
stmaddedge(graph4, "Done", "Idle")
stmentry(graph4, "Run")        ; non-default entry (id 1)
stmcompile(graph4)

instr 40
    c:k = timeinstk()

    ; ---- introspection: structure is immutable, values constant ----
    ncount:k = stmnodecount(graph4)
    ecount:k = stmedgecount(graph4)
    id_idle:k = stmnodeid(graph4, "Idle")
    id_run:k  = stmnodeid(graph4, "Run")
    id_done:k = stmnodeid(graph4, "Done")

    ; ---- current node by id, and id -> name round-trip ----
    curid:k   = stmcurrentid(graph4)
    curname:S = stmnodename(graph4, curid)   ; id -> name
    cur:S     = stmcurrent(graph4)           ; name path (cross-check)

    println("[E4] c%d  curid=%d  curname=%s  cur=%s  nodes=%d  edges=%d", c, curid, curname, cur, ncount, ecount)

    ; ---- structural assertions (hold every cycle) ----
    if ncount != 3 || ecount != 3 then
        printks("[FAIL] E4: nodes=%f edges=%f expected 3/3\n", 0, ncount, ecount)
        exitnowk(-1)
    endif
    if id_idle != 0 || id_run != 1 || id_done != 2 then
        printks("[FAIL] E4: ids I/R/D=%f/%f/%f expected 0/1/2\n", 0, id_idle, id_run, id_done)
        exitnowk(-1)
    endif
    ; id -> name must agree with the name path (stmcurrent)
    if strcmpk(curname, cur) != 0 then
        printks("[FAIL] E4: stmnodename(curid) disagrees with stmcurrent\n", 0)
        exitnowk(-1)
    endif

    ; ---- id-based drive + entry/reset ----
    if c == 1 then
        ; entry forced to Run (id 1), NOT node 0
        if curid != id_run then
            printks("[FAIL] E4 c1: entry curid=%f expected 1 (Run via stmentry)\n", 0, curid)
            exitnowk(-1)
        endif
        stmnext(graph4, id_done)             ; Run -> Done (legal, by id)
    elseif c == 2 then
        if curid != id_done then
            printks("[FAIL] E4 c2: curid=%f expected 2 (Done)\n", 0, curid)
            exitnowk(-1)
        endif
        stmnext(graph4, id_idle)             ; Done -> Idle (legal, by id)
    elseif c == 3 then
        if curid != id_idle then
            printks("[FAIL] E4 c3: curid=%f expected 0 (Idle)\n", 0, curid)
            exitnowk(-1)
        endif
        stmnext(graph4, id_done)             ; Idle -> Done ILLEGAL (no such edge)
    elseif c == 4 then
        ; illegal request must have been rejected -> still Idle
        if curid != id_idle then
            printks("[FAIL] E4 c4: curid=%f expected 0 (illegal id rejected, stay Idle)\n", 0, curid)
            exitnowk(-1)
        endif
        stmnext(graph4, id_run)              ; Idle -> Run (legal)
    elseif c == 5 then
        if curid != id_run then
            printks("[FAIL] E4 c5: curid=%f expected 1 (Run)\n", 0, curid)
            exitnowk(-1)
        endif
        stmnext(graph4, id_done)             ; Run -> Done
    elseif c == 6 then
        if curid != id_done then
            printks("[FAIL] E4 c6: curid=%f expected 2 (Done)\n", 0, curid)
            exitnowk(-1)
        endif
        stmreset(graph4)                     ; restore entry (Run) from Done
    elseif c == 7 then
        ; reset restored the entry node (Run, id 1)
        if curid != id_run then
            printks("[FAIL] E4 c7: curid=%f expected 1 (stmreset -> entry Run)\n", 0, curid)
            exitnowk(-1)
        endif
        ; explicit id -> name on a known id
        Sd:S = stmnodename(graph4, id_done)
        if strcmpk(Sd, "Done") != 0 then
            printks("[FAIL] E4 c7: stmnodename(2)=%s expected Done\n", 0, Sd)
            exitnowk(-1)
        endif
        println("[DONE] STM INTROSPECTION TEST PASSED\n")
        turnoff
    endif

    changed:k = stmadvance(graph4)

    ; illegal id transition at c3 must not have been applied
    if c == 3 && changed != 0 then
        printks("[FAIL] E4 c3: illegal id transition was applied (changed=%f)\n", 0, changed)
        exitnowk(-1)
    endif
endin

; ============================================================
; Example 5: the graph clock.
;
;   stmtick     - k-cycles elapsed since stmcompile / stmreset
;   stmtime     - graph time in seconds (advanced sample frames / sr)
;   stmnodetime - seconds since the current node became current
;
; stmadvance is what drives the clock, so the time opcodes are read
; BEFORE it: after stmadvance they already report the next cycle.
; The times are derived from integer sample-frame counters rather than
; accumulated floating-point seconds, so they cannot drift.
;
; The three invariants that are easy to get wrong:
;   1. a REJECTED stmnext must not restart the node clock
;   2. a node reads nodetime == 0 on its own first cycle
;   3. stmreset restarts tick/time/nodetime at 0, exactly like stmcompile
;
; Graph5: A <-> B, plus an edgeless C so A -> C is always illegal.
; ============================================================

graph5@global:i = stmcreate()
stmaddnode(graph5, "A")        ; id 0, entry
stmaddnode(graph5, "B")        ; id 1
stmaddnode(graph5, "C")        ; id 2, no edges: A -> C is never legal
stmaddedge(graph5, "A", "B")
stmaddedge(graph5, "B", "A")
stmcompile(graph5)

kper@global:i = ksmps / sr     ; one k-cycle in seconds

instr 50
    ; count this instrument's own passes, not k-cycles: timeinstk() is driven by
    ; the global k-counter, and an instrument that turns off earlier in the
    ; active list can make this one miss a cycle. The graph clock is driven by
    ; stmadvance, so it must be compared against passes through this code.
    c:k = init(0)
    c = c + 1

    ; ---- read the clock BEFORE stmadvance ----
    tick:k = stmtick(graph5)
    gt:k = stmtime(graph5)
    nt:k = stmnodetime(graph5)
    curid:k = stmcurrentid(graph5)

    println("[E5] c%d  curid=%d  tick=%d  time=%.6f  nodetime=%.6f", c, curid, tick, gt, nt)

    ; with a constant ksmps driver, graph time equals tick * kperiod
    if abs:k(gt - tick * kper) > 1e-9 then
        printks("[FAIL] E5: stmtime=%f expected tick*kper=%f\n", 0, gt, tick * kper)
        exitnowk(-1)
    endif

    exp_tick:k = init(0)
    exp_nt:k = init(0)
    exp_id:k = init(0)

    if c == 1 then
        exp_tick = 0            ; first cycle: the clock starts at zero
        exp_nt = 0
        exp_id = 0
    elseif c == 2 then
        exp_tick = 1
        exp_nt = kper
        exp_id = 0
    elseif c == 3 then
        exp_tick = 2
        exp_nt = 2 * kper
        exp_id = 0
    elseif c == 4 then
        exp_tick = 3            ; illegal A -> C at c3 was rejected:
        exp_nt = 3 * kper     ; the node clock kept running (invariant 1)
        exp_id = 0
    elseif c == 5 then
        exp_tick = 4            ; B became current at the end of c4, so its
        exp_nt = 0            ; first cycle reads nodetime 0 (invariant 2)
        exp_id = 1
    elseif c == 6 then
        exp_tick = 5
        exp_nt = kper
        exp_id = 1
    elseif c == 7 then
        exp_tick = 0            ; stmreset at c6 restarted the whole clock
        exp_nt = 0            ; just like stmcompile (invariant 3)
        exp_id = 0
    endif

    if tick != exp_tick then
        printks("[FAIL] E5 c%f: tick=%f expected %f\n", 0, c, tick, exp_tick)
        exitnowk(-1)
    endif
    if abs:k(nt - exp_nt) > 1e-9 then
        printks("[FAIL] E5 c%f: nodetime=%f expected %f\n", 0, c, nt, exp_nt)
        exitnowk(-1)
    endif
    if curid != exp_id then
        printks("[FAIL] E5 c%f: curid=%f expected %f\n", 0, c, curid, exp_id)
        exitnowk(-1)
    endif

    ; ---- drive ----
    if c == 3 then
        stmnext(graph5, "C")    ; ILLEGAL: no A -> C edge
    elseif c == 4 then
        stmnext(graph5, "B")    ; legal
    elseif c == 6 then
        stmreset(graph5)        ; back to entry node A, clock restarts
    elseif c == 7 then
        println("[DONE] STM CLOCK TEST PASSED\n")
        turnoff
    endif

    changed:k = stmadvance(graph5)

    if c == 3 && changed != 0 then
        printks("[FAIL] E5 c3: illegal transition A -> C was applied\n", 0)
        exitnowk(-1)
    endif
    if c == 4 && changed != 1 then
        printks("[FAIL] E5 c4: legal transition A -> B was not applied\n", 0)
        exitnowk(-1)
    endif
endin

; ============================================================
; Example 6: graph time with changing local setksmps.
;
; This guards against computing stmtime as tick * latest_kperiod. After
; 32 advances at setksmps 1 and one advance at setksmps 32, the graph has
; 33 ticks but represents 64 sample frames.
; ============================================================

graph6@global:i = stmcreate()
stmaddnode(graph6, "A")
stmcompile(graph6)

instr 60
    setksmps 1
    c:k = init(0)
    c = c + 1

    if c <= 32 then
        changed:k = stmadvance(graph6)
    endif
    if c == 32 then
        turnoff
    endif
endin

instr 61
    setksmps 32
    c:k = init(0)
    c = c + 1

    tick:k = stmtick(graph6)
    gt:k = stmtime(graph6)

    if c == 1 then
        if tick != 32 || abs:k(gt - 32 / sr) > 1e-9 then
            printks("[FAIL] E6 c1: tick=%f time=%f expected 32 and %f\n", 0, tick, gt, 32 / sr)
            exitnowk(-1)
        endif
        changed:k = stmadvance(graph6)
    elseif c == 2 then
        if tick != 33 || abs:k(gt - 64 / sr) > 1e-9 then
            printks("[FAIL] E6 c2: tick=%f time=%f expected 33 and %f\n", 0, tick, gt, 64 / sr)
            exitnowk(-1)
        endif
        println("[DONE] STM VARIABLE KSMPS CLOCK TEST PASSED\n")
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i 1  0   1 ; example 1: deterministic transitions + conditional edge
i 10 1.1 1 ; example 2: capture / analysis of random signal
i 11 2.2 1 ; example 2: graph runtime + invariant checks
i 20 3.3 1 ; example 3: instruments as stateful processes (spawn/kill via enter/exit)
i 40 4.4 1 ; example 4: introspection + id-based API + entry/reset
i 50 5.3 1 ; example 5: graph clock (tick / graph time / node time)
i 60 6.4 0.1 ; example 6: 32 one-sample advances
i 61 6.5 0.1 ; example 6: one 32-sample advance
</CsScore>
</CsoundSynthesizer>
