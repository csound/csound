<CsoundSynthesizer>
<CsInstruments>
sr = 48000
ksmps = 100
nchnls = 2

alwayson "foo", 2, 2, 2

instr foo
    print p1, p2, p3, 4
endin

instr bar
    print p1, p2, p3, p4
    asignal init 0.01
    outs asignal, asignal
endin


</CsInstruments>
<CsScore>
i "bar" 1 2 3
i "bar" 2 2 3
</CsScore>
</CsoundSynthesizer>
