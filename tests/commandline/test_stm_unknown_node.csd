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
    ; A name that is not in the compiled definition must raise a clean
    ; performance error instead of being ignored or matched partially.
    stmnext(graph, "Zeta")
    status:k, from_id:k, to_id:k = stmadvance(graph)
    printks("[FAIL] stmnext accepted an unknown node name, status=%f\n", 0, status)
    exitnowk(-1)
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
