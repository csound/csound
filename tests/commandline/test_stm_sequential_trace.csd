<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; --------------------------------------------------------------------------
; Complete sequential STM workflow
;
; Topology:
;
;   Start -> Process -> Retry -> Process -> Done
;                         \---------------> Done
;                                      Done -> Done (explicit self-edge)
;
; The graph owns control flow. WorkflowState owns application data. Each node
; is an ordinary UDO which mutates the shared struct and may request the next
; node. One supervisor dispatches the current node, advances the runner, reads
; events and observers, and prints the complete state progression.
; --------------------------------------------------------------------------

struct WorkflowState input:k, attempts:k, retries:k, result:k, completed:k, executedNode:k

opcode StartNode(runner:i, state:WorkflowState):void
    println("  opcode      : StartNode -- initialize WorkflowState")
    state.input = 2
    state.attempts = 0
    state.retries = 0
    state.result = 0
    state.completed = 0
    state.executedNode = 0
    println("  request     : StartNode -> Process")
    stmnext(runner, "Process")
endop

opcode ProcessNode(runner:i, state:WorkflowState):void
    println("  opcode      : ProcessNode -- increment attempts and choose Retry or Done")
    state.executedNode = 1
    state.attempts += 1

    if state.attempts < 2 then
        println("  request     : ProcessNode -> Retry (attempt %d)", state.attempts)
        stmnext(runner, "Retry")
    else
        state.result = state.input * 10
        println("  request     : ProcessNode -> Done (result %d)", state.result)
        stmnext(runner, "Done")
    endif
endop

opcode RetryNode(runner:i, state:WorkflowState):void
    println("  opcode      : RetryNode -- increment retries and input")
    state.executedNode = 2
    state.retries += 1
    state.input += 1
    println("  request     : RetryNode -> Process")
    stmnext(runner, "Process")
endop

opcode DoneNode(runner:i, state:WorkflowState):void
    println("  opcode      : DoneNode -- mark workflow complete when needed")
    state.executedNode = 3

    ; Emit one explicit self-transition on the first Done pass. The following
    ; pass makes no request and proves that the state remains stable.
    if state.completed == 0 then
        state.completed = 1
        println("  request     : DoneNode -> Done (explicit self-transition)")
        stmnext(runner, "Done")
    else
        println("  request     : DoneNode makes no request")
    endif
endop

opcode DispatchNode(runner:i, state:WorkflowState, current:S):void
    println("  dispatch    : current node %s", current)
    if strcmpk(current, "Start") == 0 then
        StartNode(runner, state)
    elseif strcmpk(current, "Process") == 0 then
        ProcessNode(runner, state)
    elseif strcmpk(current, "Retry") == 0 then
        RetryNode(runner, state)
    elseif strcmpk(current, "Done") == 0 then
        DoneNode(runner, state)
    endif
endop

; Immutable topology construction.
workflow_builder@global:i = stmcreate()
stmaddnode(workflow_builder, "Start")    ; id 0
stmaddnode(workflow_builder, "Process")  ; id 1
stmaddnode(workflow_builder, "Retry")    ; id 2
stmaddnode(workflow_builder, "Done")     ; id 3
stmaddedge(workflow_builder, "Start", "Process")
stmaddcondedge(workflow_builder, "Process", ["Retry", "Done"])
stmaddedge(workflow_builder, "Retry", "Process")
stmaddedge(workflow_builder, "Done", "Done")
stmentry(workflow_builder, "Start")

workflow_definition@global:i = stmcompile(workflow_builder)
workflow_runner@global:i = stminstance(workflow_definition)
workflow_state@global:WorkflowState = init(0, 0, 0, 0, 0, -1)

instr 1
    cycle:k = init(0)
    cycle += 1

    ; Snapshot before node execution.
    current_before:S = stmcurrent(workflow_runner)
    current_id_before:k = stmcurrentid(workflow_runner)
    tick_before:k = stmtick(workflow_runner)
    time_before:k = stmtime(workflow_runner)
    node_time_before:k = stmnodetime(workflow_runner)

    input_before:k = workflow_state.input
    attempts_before:k = workflow_state.attempts
    retries_before:k = workflow_state.retries
    result_before:k = workflow_state.result
    completed_before:k = workflow_state.completed

    println("\n[CYCLE %d] current %s(%d)", cycle, current_before, current_id_before)
    println("  state before: input=%d attempts=%d retries=%d result=%d completed=%d", input_before, attempts_before, retries_before, result_before, completed_before)
    println("  graph before: tick=%d time=%.6f nodeTime=%.6f", tick_before, time_before, node_time_before)

    ; Execute exactly one application node. Calls to stmnext inside nested UDOs
    ; belong to this top-level writer instrument.
    DispatchNode(workflow_runner, workflow_state, current_before)

    ; Cycle 7 demonstrates that graph reset and application-state reset are
    ; independent: stmreset returns control to Start but does not alter the
    ; WorkflowState struct.
    if cycle == 7 then
        println("  control     : supervisor calls stmreset after DoneNode")
        stmreset(workflow_runner)
    endif

    advance_status:k, advance_from:k, advance_to:k = stmadvance(workflow_runner)

    ; Snapshot after the node and advance.
    current_after:S = stmcurrent(workflow_runner)
    current_id_after:k = stmcurrentid(workflow_runner)
    tick_after:k = stmtick(workflow_runner)
    time_after:k = stmtime(workflow_runner)
    node_time_after:k = stmnodetime(workflow_runner)

    event_status:k, event_sequence:k, event_overflow:k, event_available:k, event_from:k, event_to:k = stmevent(workflow_runner)

    enter_start:k = stmonenter(workflow_runner, "Start")
    enter_process:k = stmonenter(workflow_runner, "Process")
    enter_retry:k = stmonenter(workflow_runner, "Retry")
    enter_done:k = stmonenter(workflow_runner, "Done")
    exit_start:k = stmonexit(workflow_runner, "Start")
    exit_process:k = stmonexit(workflow_runner, "Process")
    exit_retry:k = stmonexit(workflow_runner, "Retry")
    exit_done:k = stmonexit(workflow_runner, "Done")

    node_count:k = stmnodecount(workflow_runner)
    edge_count:k = stmedgecount(workflow_runner)
    done_id:k = stmnodeid(workflow_runner, "Done")
    resolved_after:S = stmnodename(workflow_runner, current_id_after)

    println("  node wrote  : input=%d attempts=%d retries=%d result=%d completed=%d executedNode=%d", workflow_state.input, workflow_state.attempts, workflow_state.retries, workflow_state.result, workflow_state.completed, workflow_state.executedNode)
    println("  advance     : status=%d from=%d to=%d", advance_status, advance_from, advance_to)
    println("  graph after : current=%s(%d) tick=%d time=%.6f nodeTime=%.6f", current_after, current_id_after, tick_after, time_after, node_time_after)

    if event_available == 1 then
        println("  event       : seq=%d status=%d from=%d to=%d overflow=%d", event_sequence, event_status, event_from, event_to, event_overflow)
    else
        println("  event       : none overflow=%d", event_overflow)
    endif

    println("  observers   : enter[S/P/R/D]=%d/%d/%d/%d exit[S/P/R/D]=%d/%d/%d/%d", enter_start, enter_process, enter_retry, enter_done, exit_start, exit_process, exit_retry, exit_done)

    ; Expected control-flow and application data for each cycle.
    expected_before:k = 0
    expected_after:k = 0
    expected_tick:k = 0
    expected_status:k = 0
    expected_from:k = 0
    expected_to:k = -1
    expected_input:k = 0
    expected_attempts:k = 0
    expected_retries:k = 0
    expected_result:k = 0
    expected_completed:k = 0
    expected_executed:k = 0
    expected_event_available:k = 1
    expected_event_status:k = 1
    expected_event_sequence:k = 0
    expected_event_from:k = 0
    expected_event_to:k = 0
    expected_enter_start:k = 0
    expected_enter_process:k = 0
    expected_enter_retry:k = 0
    expected_enter_done:k = 0
    expected_exit_start:k = 0
    expected_exit_process:k = 0
    expected_exit_retry:k = 0
    expected_exit_done:k = 0

    if cycle == 1 then
        expected_before = 0
        expected_after = 1
        expected_tick = 1
        expected_status = 1
        expected_from = 0
        expected_to = 1
        expected_input = 2
        expected_executed = 0
        expected_event_sequence = 2
        expected_event_from = 0
        expected_event_to = 1
        expected_enter_process = 1
        expected_exit_start = 1

    elseif cycle == 2 then
        expected_before = 1
        expected_after = 2
        expected_tick = 2
        expected_status = 1
        expected_from = 1
        expected_to = 2
        expected_input = 2
        expected_attempts = 1
        expected_executed = 1
        expected_event_sequence = 3
        expected_event_from = 1
        expected_event_to = 2
        expected_enter_retry = 1
        expected_exit_process = 1

    elseif cycle == 3 then
        expected_before = 2
        expected_after = 1
        expected_tick = 3
        expected_status = 1
        expected_from = 2
        expected_to = 1
        expected_input = 3
        expected_attempts = 1
        expected_retries = 1
        expected_executed = 2
        expected_event_sequence = 4
        expected_event_from = 2
        expected_event_to = 1
        expected_enter_process = 1
        expected_exit_retry = 1

    elseif cycle == 4 then
        expected_before = 1
        expected_after = 3
        expected_tick = 4
        expected_status = 1
        expected_from = 1
        expected_to = 3
        expected_input = 3
        expected_attempts = 2
        expected_retries = 1
        expected_result = 30
        expected_executed = 1
        expected_event_sequence = 5
        expected_event_from = 1
        expected_event_to = 3
        expected_enter_done = 1
        expected_exit_process = 1

    elseif cycle == 5 then
        expected_before = 3
        expected_after = 3
        expected_tick = 5
        expected_status = 3
        expected_from = 3
        expected_to = 3
        expected_input = 3
        expected_attempts = 2
        expected_retries = 1
        expected_result = 30
        expected_completed = 1
        expected_executed = 3
        expected_event_status = 2
        expected_event_sequence = 6
        expected_event_from = 3
        expected_event_to = 3
        expected_enter_done = 1
        expected_exit_done = 1

    elseif cycle == 6 then
        expected_before = 3
        expected_after = 3
        expected_tick = 6
        expected_status = 0
        expected_from = 3
        expected_to = -1
        expected_input = 3
        expected_attempts = 2
        expected_retries = 1
        expected_result = 30
        expected_completed = 1
        expected_executed = 3
        expected_event_available = 0

    elseif cycle == 7 then
        expected_before = 3
        expected_after = 0
        expected_tick = 0
        expected_status = 0
        expected_from = 0
        expected_to = -1
        expected_input = 3
        expected_attempts = 2
        expected_retries = 1
        expected_result = 30
        expected_completed = 1
        expected_executed = 3
        expected_event_status = 3
        expected_event_sequence = 7
        expected_event_from = 3
        expected_event_to = 0
        expected_enter_start = 1
        expected_exit_done = 1

    elseif cycle == 8 then
        expected_before = 0
        expected_after = 1
        expected_tick = 1
        expected_status = 1
        expected_from = 0
        expected_to = 1
        expected_input = 2
        expected_executed = 0
        expected_event_sequence = 8
        expected_event_from = 0
        expected_event_to = 1
        expected_enter_process = 1
        expected_exit_start = 1
    endif

    if current_id_before != expected_before || current_id_after != expected_after ||
       tick_after != expected_tick || advance_status != expected_status ||
       advance_from != expected_from || advance_to != expected_to then
        printks("[FAIL] cycle %d control-flow mismatch\n", 0, cycle)
        exitnowk(-1)
    endif

    if workflow_state.input != expected_input ||
       workflow_state.attempts != expected_attempts ||
       workflow_state.retries != expected_retries ||
       workflow_state.result != expected_result ||
       workflow_state.completed != expected_completed ||
       workflow_state.executedNode != expected_executed then
        printks("[FAIL] cycle %d WorkflowState mismatch\n", 0, cycle)
        exitnowk(-1)
    endif

    if event_available != expected_event_available || event_overflow != 0 then
        printks("[FAIL] cycle %d event availability mismatch\n", 0, cycle)
        exitnowk(-1)
    endif

    if expected_event_available == 1 &&
       (event_status != expected_event_status ||
        event_sequence != expected_event_sequence ||
        event_from != expected_event_from || event_to != expected_event_to) then
        printks("[FAIL] cycle %d event payload mismatch\n", 0, cycle)
        exitnowk(-1)
    endif

    if enter_start != expected_enter_start ||
       enter_process != expected_enter_process ||
       enter_retry != expected_enter_retry || enter_done != expected_enter_done ||
       exit_start != expected_exit_start ||
       exit_process != expected_exit_process ||
       exit_retry != expected_exit_retry || exit_done != expected_exit_done then
        printks("[FAIL] cycle %d observer mismatch\n", 0, cycle)
        exitnowk(-1)
    endif

    if node_count != 4 || edge_count != 5 || done_id != 3 ||
       strcmpk(current_after, resolved_after) != 0 ||
       time_after < 0 || node_time_after < 0 then
        printks("[FAIL] cycle %d introspection/time mismatch\n", 0, cycle)
        exitnowk(-1)
    endif

    if cycle == 8 then
        println("\n[PASS] complete sequential STM workflow trace")
        turnoff
    endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.05
e
</CsScore>
</CsoundSynthesizer>
