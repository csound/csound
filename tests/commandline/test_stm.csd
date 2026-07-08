<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
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
opcode stmDispatch(g:i, st:AudioState, cur:S):void
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

    ; on_enter monitors, read before advance: each fires 1 only on the
    ; cycle its node becomes current (rising-edge), 0 while it stays.
    enter_a:k = stmonenter(graph, "Analyze")
    enter_t:k = stmonenter(graph, "Texture")
    enter_f:k = stmonenter(graph, "Freeze")

    ; on_exit monitors (falling-edge): each fires 1 the cycle its node
    ; stops being current, i.e. one cycle after it requested a transition.
    exit_a:k = stmonexit(graph, "Analyze")
    exit_t:k = stmonexit(graph, "Texture")
    exit_f:k = stmonexit(graph, "Freeze")

    stmDispatch(graph, state, cur)
    changed:k = stmadvance(graph)

    ; ---- flow trace ----
    println("[E1] c%d  cur=%s  node=%d  enter[A/T/F]=%d/%d/%d  exit[A/T/F]=%d/%d/%d  changed=%d", c, cur, state.node, enter_a, enter_t, enter_f, exit_a, exit_t, exit_f, changed)

    if c == 1 then
        if state.node != 1 then
            printks("FAIL c1: node=%f expected 1 (Analyze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 1 then
            printks("FAIL c1: expected transition to Texture\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 1 || enter_t != 0 || enter_f != 0 then
            printks("FAIL c1: on_enter A/T/F=%f/%f/%f expected 1/0/0 (entry node)\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        if exit_a != 0 || exit_t != 0 || exit_f != 0 then
            printks("FAIL c1: on_exit A/T/F=%f/%f/%f expected 0/0/0 (nothing left yet)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 2 then
        if state.node != 2 then
            printks("FAIL c2: node=%f expected 2 (Texture)\n", 0, state.node)
            exitnowk(-1)
        endif
        if state.density != 40 then
            printks("FAIL c2: density=%f expected 40 (by-ref write)\n", 0, state.density)
            exitnowk(-1)
        endif
        if changed != 1 then
            printks("FAIL c2: expected transition to Freeze\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 || enter_t != 1 || enter_f != 0 then
            printks("FAIL c2: on_enter A/T/F=%f/%f/%f expected 0/1/0\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        ; Analyze was left after c1 -> its on_exit fires now
        if exit_a != 1 || exit_t != 0 || exit_f != 0 then
            printks("FAIL c2: on_exit A/T/F=%f/%f/%f expected 1/0/0 (left Analyze)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 3 then
        if state.node != 3 then
            printks("FAIL c3: node=%f expected 3 (Freeze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 1 then
            printks("FAIL c3: expected conditional transition to Analyze\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 || enter_t != 0 || enter_f != 1 then
            printks("FAIL c3: on_enter A/T/F=%f/%f/%f expected 0/0/1\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        ; Texture was left after c2 -> its on_exit fires now
        if exit_a != 0 || exit_t != 1 || exit_f != 0 then
            printks("FAIL c3: on_exit A/T/F=%f/%f/%f expected 0/1/0 (left Texture)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 4 then
        if state.node != 1 then
            printks("FAIL c4: node=%f expected 1 (Analyze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 0 then
            printks("FAIL c4: unexpected transition (no request)\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 1 || enter_t != 0 || enter_f != 0 then
            printks("FAIL c4: on_enter A/T/F=%f/%f/%f expected 1/0/0 (re-entry Analyze)\n", 0, enter_a, enter_t, enter_f)
            exitnowk(-1)
        endif
        ; Freeze was left after c3 (conditional -> Analyze) -> its on_exit fires now
        if exit_a != 0 || exit_t != 0 || exit_f != 1 then
            printks("FAIL c4: on_exit A/T/F=%f/%f/%f expected 0/0/1 (left Freeze)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
    elseif c == 5 then
        ; Analyze stays (no request). Rising-edge: on_enter must NOT refire.
        if state.node != 1 then
            printks("FAIL c5: node=%f expected 1 (Analyze)\n", 0, state.node)
            exitnowk(-1)
        endif
        if changed != 0 then
            printks("FAIL c5: unexpected transition (no request)\n", 0)
            exitnowk(-1)
        endif
        if enter_a != 0 then
            printks("FAIL c5: on_enter Analyze=%f expected 0 (no refire while staying)\n", 0, enter_a)
            exitnowk(-1)
        endif
        if exit_a != 0 || exit_t != 0 || exit_f != 0 then
            printks("FAIL c5: on_exit A/T/F=%f/%f/%f expected 0/0/0 (staying, none left)\n", 0, exit_a, exit_t, exit_f)
            exitnowk(-1)
        endif
        printks("STM TEST PASSED\n", 0)
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
    stmDispatch(graph2, state2, Scur)
    changed:k = stmadvance(graph2)

    node:k = state2.node

    ; a node always ran, so its id marker is always in {1,2,3}
    if node < 1 || node > 3 then
        printks("FAIL rnd: invalid node id %f\n", 0, node)
        exitnowk(-1)
    endif
    if changed != 0 && changed != 1 then
        printks("FAIL rnd: invalid changed flag %f\n", 0, changed)
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
            println("FAIL rnd: coverage incomplete, seen A/T/F=%d/%d/%d", seen_a, seen_t, seen_f)
            exitnowk(-1)
        endif
        if ntrans < 3 then
            println("FAIL rnd: too few transitions (%d) - drive not exercising graph", ntrans)
            exitnowk(-1)
        endif
        println("[E2] coverage: seen A/T/F=%d/%d/%d  transitions=%d", seen_a, seen_t, seen_f, ntrans)
        printks("STM RANDOM TEST PASSED\n", 0)
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
            printks("FAIL p c3: active A/B=%f/%f expected 1/0\n", 0, na, nb)
            exitnowk(-1)
        endif
        if hits_a <= 0 then
            printks("FAIL p c3: process A did not run (hits=%f)\n", 0, hits_a)
            exitnowk(-1)
        endif
    elseif c == 8 then                ; settled in B
        if na != 0 || nb != 1 then
            printks("FAIL p c8: active A/B=%f/%f expected 0/1\n", 0, na, nb)
            exitnowk(-1)
        endif
    elseif c == 13 then               ; back in A
        if na != 1 || nb != 0 then
            printks("FAIL p c13: active A/B=%f/%f expected 1/0\n", 0, na, nb)
            exitnowk(-1)
        endif
        turnoff2(30, 0, 0)
        printks("STM PROCESS TEST PASSED\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i1  0 1      ; example 1: deterministic transitions + conditional edge
i10 0 1      ; example 2: capture / analysis of random signal
i11 0 1      ; example 2: graph runtime + invariant checks
i20 0 1      ; example 3: instruments as stateful processes (spawn/kill via enter/exit)
</CsScore>
</CsoundSynthesizer>
