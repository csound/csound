<CsTest>
description = "stm rejects compiling a builder twice and releases the registry"

[expect]
exit = 1
stderr = ["[stm] stmcompile: graph already compiled"]
output_excludes = ["Segmentation fault", "AddressSanitizer", "UndefinedBehaviorSanitizer", "Abort trap"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr     = 44100
ksmps  = 32
nchnls = 1
0dbfs  = 1

instr 1
    ; The second stmcompile fails with the registry locked by the builder
    ; lookup. It must unlock it: cleanup takes the same lock, so a leaked
    ; one hangs Csound instead of letting it exit with the init error.
    builder:i = stmcreate()
    stmaddnode(builder, "A")
    definition:i = stmcompile(builder)
    again:i = stmcompile(builder)
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
