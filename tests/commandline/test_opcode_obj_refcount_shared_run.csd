<CsTest>
description = "two aliases of one opcode object rebind their arguments per run"

[expect]
exit = 0
output = ["shared run correct:"]
output_excludes = ["shared run wrong:", "not initialised", "Segmentation fault", "AddressSanitizer"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 10

opcode Twice(x:k):k
  xout x * 2
endop

instr 1
  shared:Opcode = create(opcoderef("Twice"))
  alias:Opcode = shared
  a:k run shared, 1
  b:k run alias, 3
  done:k init 0
  if done == 0 then
    if abs(a-2) > 0.01 || abs(b-6) > 0.01 then
      printks "shared run wrong: %g %g\n", 1, a, b
    else
      printks "shared run correct: %g %g\n", 1, a, b
    endif
    done = 1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.2
e
</CsScore>
</CsoundSynthesizer>
