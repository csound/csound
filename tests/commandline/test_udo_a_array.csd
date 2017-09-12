<CsoundSynthesizer>
<CsInstruments>
ksmps = 32

opcode test1, k[], 0
karr[] init 5
xout karr
endop

opcode test2, a[], 0
aarr[] init 5
xout aarr
endop

instr 1
kbla[] test1 ;ok
abla[] test2 ;error
ilen = lenarray(kbla)
print ilen
ilen = lenarray(abla)
print ilen
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
