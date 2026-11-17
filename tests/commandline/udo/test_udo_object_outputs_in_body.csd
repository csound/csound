<CsTest>
description = "A by-reference UDO gives its argument to an Opcode object call with several outputs"

[expect]
exit = 0
output = ["object outputs checks done"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

gkStored init 0

opcode Store(value:k):void
  gkStored = value + 1
endop

opcode One(value:k):k
  first:k = value + 1
  xout first
endop

opcode Pair(value:k):(k,k)
  first:k = value + 1
  second:k = value + 2
  xout first, second
endop

opcode Triple(value:k):(k,k,k)
  first:k = value + 1
  second:k = value + 2
  third:k = value + 3
  xout first, second, third
endop

opcode PairI(value:i):(i,i)
  first:i = value + 1
  second:i = value + 2
  xout first, second
endop

; init, perf and run declare their outputs as "*": the argument comes after
; however many outputs the call gives, and after the object. With none, the
; "*" still holds one slot.
opcode NoOut(val:k):void
  objects:Opcode[] = create(opcoderef("Store"), 1)
  init objects[0], val
  perf objects[0], val
endop

opcode OneOut(val:k):k
  objects:Opcode[] = create(opcoderef("One"), 1)
  kA init objects[0], val
  kA perf objects[0], val
  xout kA
endop

opcode TwoOut(val:k):(k,k)
  objects:Opcode[] = create(opcoderef("Pair"), 1)
  kA, kB init objects[0], val
  kA, kB perf objects[0], val
  xout kA, kB
endop

opcode ThreeRun(val:k):(k,k,k)
  objects:Opcode[] = create(opcoderef("Triple"), 1)
  kA, kB, kC run objects[0], val
  xout kA, kB, kC
endop

opcode TwoInit(val:i):(i,i)
  objects:Opcode[] = create(opcoderef("PairI"), 1)
  iA, iB init objects[0], val
  xout iA, iB
endop

instr 1
  iX, iY = TwoInit(20)
  if iX != 21 || iY != 22 then
    prints "FAILED: two i-rate outputs gave %g %g\n", iX, iY
    exitnow -1
  endif

  kCycle timeinstk
  kV init 10
  kV += 1
  NoOut(kV)
  kOne = OneOut(kV)
  kA, kB = TwoOut(kV)
  kP, kQ, kR = ThreeRun(kV)
  if gkStored != kV + 1 then
    printks "FAILED: no output stored %g for %g\n", 0, gkStored, kV
    exitnowk -1
  endif
  if kOne != kV + 1 then
    printks "FAILED: one output gave %g for %g\n", 0, kOne, kV
    exitnowk -1
  endif
  if kA != kV + 1 || kB != kV + 2 then
    printks "FAILED: two outputs gave %g %g for %g\n", 0, kA, kB, kV
    exitnowk -1
  endif
  if kP != kV + 1 || kQ != kV + 2 || kR != kV + 3 then
    printks "FAILED: three outputs gave %g %g %g for %g\n", 0, kP, kQ, kR, kV
    exitnowk -1
  endif

  if kCycle == 4 then
    printks "object outputs checks done\n", 0
    turnoff
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 .25
e
</CsScore>
</CsoundSynthesizer>
