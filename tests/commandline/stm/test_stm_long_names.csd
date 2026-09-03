<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

; Node names are unbounded strings copied into the immutable definition.
; stmcurrent and stmnodename grow their STRINGDAT outputs at performance time
; when the name does not fit, so a 621-character name exercises that growth
; path deterministically, together with the long-name lookup used by stmnext
; and stmnodeid.

chunk@global:S = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
long_name@global:S = "L"
idx@global:i = 0
while idx < 10 do
    long_name = strcat(long_name, chunk)
    idx += 1
od

builder@global:i = stmcreate()
stmaddnode(builder, long_name)
stmaddnode(builder, "B")
stmaddedge(builder, long_name, "B")
stmaddedge(builder, "B", long_name)
definition@global:i = stmcompile(builder)
runner@global:i = stminstance(definition)

instr 1
    cycle:k = init(0)
    cycle += 1

    current:S = stmcurrent(runner)
    current_id:k = stmcurrentid(runner)
    resolved:S = stmnodename(runner, current_id)
    long_id:k = stmnodeid(runner, long_name)

    ; every cycle: the long name resolves to id 0 and both string outputs agree
    if long_id != 0 then
        printks("[FAIL] long names: stmnodeid=%f expected 0\n", 0, long_id)
        exitnowk(-1)
    endif
    if strcmpk(current, resolved) != 0 then
        printks("[FAIL] long names: stmcurrent and stmnodename disagree\n", 0)
        exitnowk(-1)
    endif

    if cycle == 1 then
        if current_id != 0 || strcmpk(current, long_name) != 0 then
            printks("[FAIL] long names: initial node mismatch, id=%f\n", 0, current_id)
            exitnowk(-1)
        endif
        stmnext(runner, "B")
        status:k, from_id:k, to_id:k = stmadvance(runner)
    elseif cycle == 2 then
        if current_id != 1 || strcmpk(current, "B") != 0 then
            printks("[FAIL] long names: expected node B, id=%f\n", 0, current_id)
            exitnowk(-1)
        endif
        ; request the transition back through the long-name lookup
        stmnext(runner, long_name)
        status:k, from_id:k, to_id:k = stmadvance(runner)
    elseif cycle == 3 then
        if current_id != 0 || strcmpk(current, long_name) != 0 then
            printks("[FAIL] long names: return to long node failed, id=%f\n", 0, current_id)
            exitnowk(-1)
        endif
        printks("[PASS] stm long node names\n", 0)
        turnoff
    endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
