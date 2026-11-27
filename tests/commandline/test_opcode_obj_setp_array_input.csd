<CsTest>
description = "a k[] setp override is stored as a full owned ARRAYDAT instead of a scalar-sized value"

[expect]
exit = 0
output = ["array override: 7"]
output_excludes = ["Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Total(values:k[]):k
  xout sumarray(values)
endop

instr 1
  object:Opcode create opcoderef("Total")
  given:k[] fillarray 1, 2
  override:k[] fillarray 3, 4
  setp object, 0, override
  result:k run object, given
  printks "array override: %g\n", 0, result
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
