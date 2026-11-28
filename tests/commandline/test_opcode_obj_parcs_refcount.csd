<CsTest>
description = "parallel note copies of a shared Opcode object keep a stable dataspace reference"

[expect]
exit = 0
output = ["parcs opcode refs ok"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 -j8
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

first@global:Opcode = create(opcoderef("lincos"))
second@global:Opcode = create(opcoderef("lincos"))

instr 10
  initial:k run first, 0.5, 0, 10
  initial2:k run second, 0.5, 0, 10
endin

instr 1
  local:Opcode = first
  flip:k init 0
  if flip == 0 then
    local = second
  else
    local = first
  endif
  flip = 1 - flip
  value:k run local, 0.5, 0, 10
endin

instr 100
  printks "parcs opcode refs ok\n", 0
  turnoff
endin
</CsInstruments>
<CsScore>
i 10 0 0.01
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 1 0.02 0.2
i 100 0.3 0.1
e 0.5
</CsScore>
</CsoundSynthesizer>
