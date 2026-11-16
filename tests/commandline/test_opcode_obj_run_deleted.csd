<CsTest>
description = "run on an Opcode object, or an Opcode[], that another note deleted while it played is refused"

[expect]
exit = 2
stderr = ["opcode object not initialised"]
output = ["the next note ran"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10
0dbfs = 1
shared@global:Opcode = create(opcoderef("oscili"))
sharedArray@global:Opcode[] = create(opcoderef("oscili"), 2)

; deletes the objects when it ends, while instr 2 and 3 run them
instr 1
  delete shared
  delete sharedArray
endin

instr 2
  signal:a run shared, 0.5, 100
endin

instr 3
  amps:k[] fillarray 0.5, 0.25
  signals:a[] init 2
  signals run sharedArray, amps, 100
endin

instr 4
  prints "the next note ran\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
i 2 0 0.3
i 3 0 0.3
i 4 0.4 0.1
e
</CsScore>
</CsoundSynthesizer>
