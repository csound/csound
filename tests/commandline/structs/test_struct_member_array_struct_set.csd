<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

0dbfs = 1

struct Item value:i
struct Holder values:Item[]

instr 1
  values:Item[] init 1
  holder:Holder init values
  item:Item init 42

  holder.values[0] = item

  assertEquals(holder.values[0].value, 42)
endin
</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
