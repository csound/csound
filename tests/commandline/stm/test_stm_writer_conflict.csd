<CsoundSynthesizer>
<CsOptions>
-n -j2
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

builder@global:i = stmcreate()
stmaddnode(builder, "A")
definition@global:i = stmcompile(builder)
runner@global:i = stminstance(definition)

instr 1 ; first active writer owns the runner claim
    status:k, from_id:k, to_id:k = stmadvance(runner)
endin

instr 2 ; an overlapping writer must be rejected during init
    stmreset(runner, 1)
endin
</CsInstruments>
<CsScore>
i 1 0    0.10
i 2 0.01 0.01
e
</CsScore>
</CsoundSynthesizer>
