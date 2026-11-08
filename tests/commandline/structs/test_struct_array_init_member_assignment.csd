<CsTest>
description = "init-rate struct array member writes preserve copied values"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
0dbfs = 1
#include "../libassert.orc"

struct Field value:i, enabled:b
struct Record field:Field, other:i

opcode SetValue(records:Record[], index:i, value:i):Record[]
  changed:Record[] = records
  changed[index].field.value = value
  changed[index].field.enabled = (value > 0)
  xout changed
endop

instr 1
  original:Record[] init 2
  original[0].field.value = 3
  original[0].other = 42
  original[1].field.value = 5

  index:i = 0
  changed:Record[] = SetValue(original, index, 9)
  changed[1].field.value = changed[0].field.value + 1

  assertEquals(original[0].field.value, 3)
  assertEquals(original[1].field.value, 5)
  assertEquals((original[0].field.enabled ? 1 : 0), 0)
  assertEquals(changed[0].field.value, 9)
  assertEquals(changed[1].field.value, 10)
  assertEquals(changed[0].other, 42)
  assertEquals((changed[0].field.enabled ? 1 : 0), 1)
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
