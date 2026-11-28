<CsTest>
description = "setp reaches a pass-by-ref UDO body and survives a shared-object rebind"

[expect]
exit = 0
output = ["setp first: 2", "setp override: 10", "setp rebound: 10"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Twice(value:k):k
  xout value * 2
endop

instr 1
  object:Opcode create opcoderef("Twice")
  alias:Opcode = object
  one:k init 1
  three:k init 3
  override:k init 5
  first:k run object, one
  setp object, 0, override
  overridden:k run object, one
  rebound:k run alias, three
  done:k init 0
  if done == 0 then
    printks "setp first: %g\n", 0, first
    printks "setp override: %g\n", 0, overridden
    printks "setp rebound: %g\n", 0, rebound
    done = 1
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
e
</CsScore>
</CsoundSynthesizer>
