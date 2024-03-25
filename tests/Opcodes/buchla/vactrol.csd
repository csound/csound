<CsoundSynthesizer>

<CsInstruments>
        nchnls = 2
        0dbfs = 1
        
    instr 1
        a1 lfo 0.3, 1, 4
        a2 vactrol a1
        a3 oscili 2, 440
           out     a1*a3,a2*a3
     endin
                    
</CsInstruments>

<CsScore>
    i1 0 3
    e
</CsScore>

</CsoundSynthesizer>
