<CsTest>
description = "UDO rank checks preserve uppercase letters in struct type names"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
struct Key value:i

opcode Forward(values:Key[*]):Key[*]
  xout values
endop

instr 1
  source:Key[][] init 2, 3
  value:Key init 17
  source[1][2] = value
  copy:Key[][] = Forward(source)
  if copy[1][2].value != 17 then
    exitnow(1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
