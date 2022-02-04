<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType imaginary:i, real:i

opcode processMyType(dummy:i[], input:MyType[]):i

ilen = lenarray(input)
print ilen
indx = 0
while (indx < ilen) do
  temp:MyType = input[indx]
  print temp.imaginary
  print temp.real
  indx += 1
od
indx = 0

xout 0
endop

instr 1	

var0:MyType[] init 4

indx = 0

while (indx < 4) do

  temp:MyType = var0[indx]
  temp.imaginary = (indx * 2)
  temp.real = (indx + 1) * 2
  var0[indx] = temp

  /* FIXME */
  /*var0[indx].imaginary = (indx * 2) * 2*/
  /*var0[indx].real = ((indx + 1) * 2) * 2*/

  indx += 1

od
indx = 0

idummy[] init 2
itest = processMyType(idummy, var0)

endin

</CsInstruments>
; ==============================================
<CsScore>
i1 0 0.5


</CsScore>
</CsoundSynthesizer>

