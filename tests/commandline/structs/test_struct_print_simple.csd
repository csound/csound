<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType imaginary:i, real:i

instr 1

temp:MyType init 10, 20

printks "imaginary: %d\n", 0, temp.imaginary
printks "real: %d\n", 0, temp.real

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5


</CsScore>
</CsoundSynthesizer>