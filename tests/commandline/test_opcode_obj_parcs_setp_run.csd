<CsTest>
description = "parallel notes repeatedly setp and run a shared Opcode without using freed setp storage"

[expect]
exit = 0
output = ["run sum:", "setp sum:", "parcs setp run ok"]
output_excludes = ["Segmentation fault", "AddressSanitizer", "heap-use-after-free"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -j2
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

shared@global:Opcode = create(sqrt, 1)

instr 1
  sum:k init 0
  source:k init 0.5
  value:k run shared, source
  sum = sum + value
  printks "run sum: %g\n", 0, sum
endin

instr 2
  override:k init 0.25
  override = override + 0.001
  setp shared, 0, override
  printks "setp sum: %g\n", 0, override
endin

instr 100
  printks "parcs setp run ok\n", 0
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.3
i 1 0 0.3
i 2 0 0.3
i 2 0 0.3
i 100 0 0.05
e
</CsScore>
</CsoundSynthesizer>
