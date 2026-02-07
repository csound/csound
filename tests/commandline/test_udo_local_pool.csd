<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
0dbfs = 1

Sresult = "anything"

opcode test1, S[], 0
       Sresult[] fillarray "t1", "t2"
       xout Sresult
endop

instr 1
       Sresult[] test1
endin


</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>


