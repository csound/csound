<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

builder@global:i = stmcreate()
stmaddnode(builder, "A")
stmaddnode(builder, "B")
stmaddedge(builder, "A", "B")
definition@global:i = stmcompile(builder)
graph@global:i = stminstance(definition)

instr 1
    ; Fractional node ids must not be silently truncated to integers.
    stmnext(graph, 1.5)
    status:k, from_id:k, to_id:k = stmadvance(graph)
    printks("[FAIL] stmnext.id accepted a fractional node id, status=%f\n", 0, status)
    exitnowk(-1)
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
