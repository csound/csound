<CsTest>
description = "Rebinding an Opcode object to a string literal its argument cannot hold is a performance error, not an allocation"

[expect]
exit = "nonzero"
stderr = ["could not rebind arguments"]
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
  ; 64 characters and a zero: one byte past the 64 the variable has from init.
  perf objects[0], "a sixty-four character literal is one more than this variable ha"
endin
</CsInstruments>
<CsScore>
i 1 0 .1
e
</CsScore>
</CsoundSynthesizer>
