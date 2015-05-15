<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType imaginary:k, real:k
struct MyType2 complex_num:MyType, name:S

instr 1	

var0:MyType init 1, 2
var1:MyType2 init var0, "MyType2 Test" 

printks "var0) i %d r %d\n", 0.2, var0.imaginary, var0.real

printks "var1) i %d r %d\n", 0.2, var1.complex_num.imaginary, \
        var1.complex_num.real
      
prints var1.name 

; TO FIX
var0.imaginary += 1
var0.real += 1
var1.complex_num.imaginary += 2
var1.complex_num.real += 2

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5


</CsScore>
</CsoundSynthesizer>

