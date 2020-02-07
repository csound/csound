

<CsoundSynthesizer>
<CsOptions>
-odac -d
</CsOptions>
<CsInstruments>
sr = 44100 
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1
    icutoff = p4
    ires = p5
    imode = p6
    aout rezzy pinkish(0.5), icutoff, ires, imode
    out aout
endin

instr 2
    kcutoff line 1, p3, 150
    kres line 10, p3, 1000
    imode = 0
    aout rezzy pinkish(0.5), kcutoff, kres, imode
    out aout
endin

</CsInstruments>
<CsScore>
;x
i 1 0 1 132 100 0 ; under 133Hz lowpass resonant filter explodes
i 1 0 1 132 1 0 ; and it does not depend on amount of the resonance
s
i 2 0 5

s
i 1 0 1 13350 100 1 ; above 13350Hz highpass resonant filter explodes
</CsScore>
</CsoundSynthesizer>

