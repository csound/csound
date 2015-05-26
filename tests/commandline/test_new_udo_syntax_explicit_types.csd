<CsoundSynthesizer>
<CsInstruments>

sr	=	44100
ksmps	=	1
nchnls	=	2
0dbfs	=	1

struct MyType val0:i, val1:i

opcode testFunc(val:MyType):void
  print val.val0
  print val.val1
  xout 0
endop

instr 1

testVal:MyType init 8, 88
testFunc(testVal) 

turnoff

endin

</CsInstruments>
<CsScore>
i1 0 0.2 
</CsScore>
</CsoundSynthesizer>

