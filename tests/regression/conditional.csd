<CsoundSynthesizer>

<CsInstruments>

instr 1
 i_value = 0
 i_value2 = 0
 i_value = (i_value2 == 0) ? i_value :\
           (i_value < 0) ? (1 / floor(i_value)) :\
           (i_value > 0) ? (1 / ceil(i_value)) : 0

 print i_value
endin


</CsInstruments>

<CsScore>
i1 0 0.1
e
</CsScore>

</CsoundSynthesizer>
