<CsTest>
description = "a boolean or number conversion for xout does not make an init-only UDO run at performance"

[expect]
exit = 0
output = ["pass recursive", "pass nested"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 32

; a call skipped at init must not run at performance, uninitialised
opcode Recursive(depth:i):b
  if depth > 0 ithen
    child:b = Recursive(depth - 1)
  endif
  xout 1
endop

opcode Leaf():b
  xout 1
endop

opcode Middle():b
  held:b = Leaf()
  xout 1
endop

opcode Outer(depth:i):b
  if depth > 0 ithen
    child:b = Middle()
  endif
  xout 1
endop

instr 1
  result:b = Recursive(2)
  value:k = max(0, result)
  if timeinstk() == 5 then
    printf("%s recursive\n", 1, value == 1 ? "pass" : "fail")
    turnoff
  endif
endin

instr 2
  result:b = Outer(0)
  value:k = max(0, result)
  if timeinstk() == 5 then
    printf("%s nested\n", 1, value == 1 ? "pass" : "fail")
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i1 0 0.01
i2 0 0.01
</CsScore>
</CsoundSynthesizer>
