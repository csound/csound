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

  ; Read a whole struct from an array stored in a struct member.
  copyIndex:k init 1
  selected:Box init holder.items[copyIndex]
  assertEquals(selected.value, 20)

  ; Keep the matching performance read out of the init pass.
  perfIndex:k init -1
  perfIndex = 0
  perfValue:k = holder.items[perfIndex].value
  if (timeinstk() == 1) then
    event "i", 2, 0, 0.001, perfValue, 10
  endif

  prints "Struct-array member with scalar test passed\n"
endin

instr 2
  assertEquals(p4, p5)
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
e
</CsScore>
</CsoundSynthesizer>
