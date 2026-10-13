<CsTest>
description = "UDO rate settings must precede ordinary opcode initialization"

[expect]
exit = "nonzero"
stderr = ["setksmps must precede opcode initialization", "oversample must precede opcode initialization", "undersample must precede opcode initialization"]
output = ["No UDO body ran before the error"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 8
nchnls = 1
giBodyStarts init 0

; Reject the ordering before any of these side effects can run.
opcode LateBlock, i, 0
  giBodyStarts += 1
  setksmps 2
  xout ksmps
endop

opcode LateOver():i
  giBodyStarts += 1
  oversample 2
  xout sr
endop

opcode LateUnder():i
  giBodyStarts += 1
  undersample 2
  xout sr
endop

instr 1
  iBlock LateBlock
endin

instr 2
  iRate LateOver
endin

instr 3
  iRate LateUnder
endin

instr CheckNoBody
  if giBodyStarts == 0 then
    prints "No UDO body ran before the error\n"
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .25
i 2 0 .25
i 3 0 .25
i "CheckNoBody" .25 .25
</CsScore>
</CsoundSynthesizer>
