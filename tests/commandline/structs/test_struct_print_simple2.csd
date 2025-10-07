<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType imaginary:i, real:i

instr 1

temp:MyType init

temp.imaginary = 10
temp.real = 20

ival = temp.imaginary
print ival

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5


</CsScore>
</CsoundSynthesizer>