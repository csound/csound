<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1
0dbfs = 1

#include "../libassert.orc"

struct Box value:i
struct Holder items:Box[], length:i

instr 1
  boxes:Box[] init 2

  first:Box init 10
  second:Box init 20
  boxes[0] = first
  boxes[1] = second

  holder:Holder init boxes, 2

  copied:Box[] = holder.items

  assertEquals(holder.length, 2)
  assertEquals(lenarray:i(copied), 2)
  assertEquals(copied[0].value, 10)
  assertEquals(copied[1].value, 20)

  prints "Struct-array member with scalar test passed\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
