<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType  imaginary:k, real:k, kimaginary, kreal
struct MyType2 x:i, y:i

opcode processMyType(in:MyType):(MyType)
  retVal:MyType init 0, 0, 0, 0
  retVal.imaginary = in.imaginary + 1
  retVal.real = in.real + 1
  retVal.kimaginary = in.kimaginary + 1
  retVal.kreal = in.kreal + 1
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

;; make sure that struct references
;; refer correctly to their original pointer
instr 2

  iexitCode = 0

  var1:MyType2 init 1, 2
  var2:MyType2 = var1
  var2.x = 6
  var2.y = 7

  iexpect6 = var1.x
  iexpect7 = var1.y

  if iexpect6 != 6 then
    iexitCode = 1
  endif

  if iexpect7 != 7 then
    iexitCode = 1
  endif

  if iexitCode == 0 then
    prints "struct reference test success\n"
  else
    prints "struct reference test failed!\n"
    exitnow 1
  endif

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5
i2 + 0


</CsScore>
</CsoundSynthesizer>
