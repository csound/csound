<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

graph@global:i = stmcreate()
stmaddnode(graph, "A")
stmaddnode(graph, "B")
stmaddedge(graph, "A", "B")
stmcompile(graph)

instr 1
    ; Fractional node ids must not be silently truncated to integers.
    stmnext(graph, 1.5)
    changed:k = stmadvance(graph)
    printks("[FAIL] stmnext.id accepted a fractional node id, changed=%f\n", 0, changed)
    exitnowk(-1)
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
