<CsTest>
description = "a UDO returning a changing k input as a boolean follows it, as its signature makes it run at performance"

[expect]
exit = 0
output = ["pass k input"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
ksmps = 32

opcode Bool(value:k):b
  xout value
endop

instr 1
  value:k = timeinstk() % 2
  flag:b = Bool(value)
  seen:k = max(0, flag)
  mismatches:k init 0
  if seen != value then
    mismatches += 1
  endif
  if timeinstk() == 8 then
    if mismatches == 0 then
      printf("pass k input\n", 1)
    else
      printf("k input failed: %g mismatches\n", 1, mismatches)
    endif
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i1 0 0.01
</CsScore>
</CsoundSynthesizer>
