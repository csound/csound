<CsTest>
description = "repeated scalar setp calls keep typed, live override storage across control cycles"

[expect]
exit = 0
output = ["scalar: 10", "scalar end: 10"]
output_excludes = ["could not set arg", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

object@global:Opcode = create(opcoderef("lincos"))

instr 1
  input:k init 1
  start:k init 0
  limit:k init 10
  override:k init 5
  count:k init 0
  result:k init 0
  setp object, 0, override
  result run object, input, start, limit
  count = count + 1
  if count == 1 then
    printks "scalar: %g\n", 0, result
  endif
  if count == 4 then
    printks "scalar end: %g\n", 0, result
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
