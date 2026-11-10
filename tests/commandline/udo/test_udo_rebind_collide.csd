<CsTest>
description = "Rebinding an Opcode object at performance to a longer variable that is both its input and its output still works"

[expect]
exit = 0
output = ["Sb is a long text, longer than the sixty-four bytes the input variable has from init, by far"]
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

opcode Echo(Sin:S):S
  Sout strcpyk Sin
  xout Sout
endop

instr 1
  ; Sa and Sb are each both the input and the output of the object.
  objects:Opcode[] = create(opcoderef("Echo"), 1)
  Sa strcpy "short"
  Sb strcpy "a long text, longer than the sixty-four bytes the input variable has from init, by far"
  Sa init objects[0], Sa
  kCycle timeinstk
  if kCycle == 1 then
    Sb perf objects[0], Sb
    printf "Sb is %s\n", 1, Sb
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 .1
e
</CsScore>
</CsoundSynthesizer>
