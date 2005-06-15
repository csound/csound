<CsoundSynthesizer>

<CsInstruments>
instr 1
    OSCinit     7070
endin

instr 2
        ki init 0       
    kk OSClisten "/foo/bar", "i", ki
endin

</CsInstruments>

<CsScore>
i1 0 1
i2 0 10
e
</CsScore>

</CsoundSynthesizer>
