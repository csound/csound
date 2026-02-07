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

opcode test3, "i[]i", "i[]"
i1[] xin
ivar[] = i1
i2 = 1
xout ivar, i2
endop

instr 2
ivar[] init 4
i1[] = [1,2,3]
ivar[],i2 = test3(i1)
print ivar[0]
endin

</CsInstruments>
<CsScore>
i 1 0 1
i 2 0 1
</CsScore>
</CsoundSynthesizer>
