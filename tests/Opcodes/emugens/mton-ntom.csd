<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

instr 1
    S4 mton ntom("7D+63")
    puts S4, 1

    S1  mton 60
    printf_i "midi 60 = %s \n", 1, S1
    
    S2 mton ftom(442)
    printf_i "442 Hz = %s \n", 1, S2
    
    S3 = mton(48.25)
    printf_i "midi 48.25 = %s \n", 1, S3
    
    k1 = ntom("4C")
    printf_i "4C = midi %f \n", 1, k1
    
    i2  ntom "4E"
    printf_i "4E = %f \n", 1, i2
    
    S5 = mton(ntom("4G+"))
    printf_i "roundtrip 4G+: %s \n", 1, S5
    
    turnoff
endin

instr 2
    ; test i-time and k-time execution
    k1 = ntom("4Eb-31")
    printf "4Eb-31 = %f \n", 1, k1

    i0  ntom "4C+"
    printf_i "4C+ = %f \n", 1, i0

    i1 = ntom:i("4A")
    printf_i "4A = %f \n", 1, i1
    turnoff
endin

</CsInstruments>
<CsScore>

i 1 0 1
i 2 0 1

</CsScore>
</CsoundSynthesizer>
