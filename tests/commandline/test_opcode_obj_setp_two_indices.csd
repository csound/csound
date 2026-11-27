<CsTest>
description = "setp overrides on two distinct indices stay stable across successive control cycles"

[expect]
exit = 0
output = ["sum: 3"]
output_excludes = ["could not set arg", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Sum2(a:k, b:k):k
  xout a + b
endop

instr 1
  object:Opcode create opcoderef("Sum2")
  callA:k init 10
  callB:k init 20
  overrideA:k init 1
  overrideB:k init 2
  count:k init 0
  out:k init 0
  if count == 0 then
    setp object, 0, overrideA
  else
    setp object, 1, overrideB
  endif
  out run object, callA, callB
  count = count + 1
  if count == 2 then
    printks "sum: %g\n", 0, out
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
