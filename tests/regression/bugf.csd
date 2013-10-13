<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
    sr = 44100
    ksmps = 256
    nchnls = 2
    0dbfs = 1

    giSine        ftgen    0, 0, 8, 10, 1
    giEmpty        ftgen    0, 0, 8, 2, 0
    giEmpty2    ftgen    0, 0, 8, 2, 0

    instr     1
    ftsave "testTable.tab", 1, giSine
    endin

    instr     2
    ftload "testTable.tab", 1, giEmpty
    endin

    instr     3
    ftsave "testTable2.tab", 1, giSine, giSine
    endin

    instr     4
    ftload "testTable2.tab", 1, giEmpty, giEmpty2
    endin

</CsInstruments>
<CsScore>
;i 3     1     1
;i 4     3     1
i 1     5     1
i 2     7     1
e
</CsScore>
</CsoundSynthesizer>
