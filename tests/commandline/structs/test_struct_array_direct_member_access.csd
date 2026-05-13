<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 2
0dbfs = 1

#include "../libassert.orc"

; Guards issue #2529: direct reads like array[i].member must read the
; addressed element, not stringify to array.member and drop the index.
struct MyType val1:i, val2:i

instr 1
  array:MyType[] init 2
  array[0].val1 = 1
  array[0].val2 = 2
  array[1].val1 = 10
  array[1].val2 = 20

  ; Read both members from two different elements to prove the index survives.
  ival1 = array[0].val1
  assertEquals(ival1, 1)

  ival2 = array[0].val2
  assertEquals(ival2, 2)

  ival3 = array[1].val1
  assertEquals(ival3, 10)

  ival4 = array[1].val2
  assertEquals(ival4, 20)

  ; Existing workaround path: copying the element first should still work.
  var:MyType = array[0]
  ival5 = var.val1
  assertEquals(ival5, 1)

  prints "Struct array direct member access test passed!\n"
endin

</CsInstruments>

<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
