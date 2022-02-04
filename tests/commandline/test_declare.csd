<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

struct TypeX val1:i

declare myFun(arg1:TypeX):(TypeX)

instr 1
  varX:TypeX init 1
  varY:TypeX = myFun(varX)
  print(varY.val1)
endin

opcode myFun(arg1:TypeX):TypeX
  retVal:TypeX init arg1.val1 + 1
  xout(retVal)
endop

</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
