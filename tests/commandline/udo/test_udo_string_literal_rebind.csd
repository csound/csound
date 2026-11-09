<CsTest>
description = "Rebinding an Opcode object to a string literal its argument cannot hold is a performance error, not an allocation"

[expect]
exit = "nonzero"
stderr = ["could not rebind arguments"]
output = ["the body read a literal that fits"]
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

opcode Show(Sx:S):void
  printf "%s\n", 1, Sx
endop

instr 1
  objects:Opcode[] = create(opcoderef("Show"), 1)
  init objects[0], "short"
  perf objects[0], "the body read a literal that fits"
endin

instr 2
  objects:Opcode[] = create(opcoderef("Show"), 1)
  init objects[0], "short"
  ; Longer than the 64 bytes the argument's variable has from init.
  perf objects[0], "a literal longer than the sixty-four bytes the variable has from init, which it cannot grow to hold"
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 2 .2 .1
e
</CsScore>
</CsoundSynthesizer>
