<CsoundSynthesizer>
;<CsOptions>
;</CsOptions>
; ==============================================
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

opcode Test,a,ak
a1, k1 xin
a2 = a1*k1
xout a2
endop

opcode Test,k,kk
k1, k2 xin
k2 = k1*k2
xout k2
endop

instr 1
a2 diskin2 "fox.wav",1
a1 Test a2,1
out a1
endin


</CsInstruments>
<CsScore>
i1 0 .5
</CsScore>
</CsoundSynthesizer>

