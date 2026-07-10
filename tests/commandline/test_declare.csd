<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

struct TypeX val1:i
struct TypeY val1:i

declare myFun(arg1:TypeX, amount:i):(TypeY)

instr 1
  varX:TypeX init 1
  varY:TypeY = myFun(varX, 2)

  if (varY.val1 != 3) then
    prints "declare signature test failed: expected 3, got %d\n", varY.val1
    exitnow(1)
  else
    prints "declare signature test passed\n"
  endif
endin

opcode myFun(arg1:TypeX, amount:i):TypeY
  retVal:TypeY init arg1.val1 + amount
  xout(retVal)
endop

</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
