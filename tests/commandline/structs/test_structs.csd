<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType imaginary:k, real:k, kimaginary, kreal

opcode processMyType(var:MyType):(MyType)
  retVal:MyType init 0, 0, 0, 0
  retVal.imaginary = var.imaginary + 1 
  retVal.real = var.real + 1 
  retVal.kimaginary = var.kimaginary + 1
  retVal.kreal = var.kreal + 1 
  xout retVal
endop

instr 1	

var0:MyType init 1, 2, 3, 4 
;var1:MyType = init:MyType(0, 0, 1, 1)

var0.imaginary init 5
var0.real init 6
var0.kimaginary init 7
var0.kreal init 9

var0.imaginary += 1 
var0.real += 2 
var0.kimaginary += 3 
var0.kreal += 4 

var2:MyType processMyType var0

; this does not work yet...
;var2:MyType = processMyType(var0)

printks "i %d r %d ki %d kr %d\n", 0.2, var0.imaginary, var0.real, var0.kimaginary, var0.kreal
printks "\ti %d r %d ki %d kr %d\n", 0.2, var2.imaginary, var2.real, var2.kimaginary, var2.kreal

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5


</CsScore>
</CsoundSynthesizer>

