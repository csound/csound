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

; Guards nested lowering for array-rooted struct paths such as
; array[i].inner.val on both write and read.
struct Inner val:i
struct Outer inner:Inner

instr 1
  array:Outer[] init 2

  ; Write through a nested member path on two elements.
  array[0].inner.val = 7
  array[1].inner.val = 9

  ; Read through the same nested path and verify the index still matters.
  i0 = array[0].inner.val
  assertEquals(i0, 7)

  i1 = array[1].inner.val
  assertEquals(i1, 9)

  ; The array getter must run at init before the nested aggregate copy.
  copyIndex:k init 1
  copy:Inner init array[copyIndex].inner
  assertEquals(copy.val, 9)

  prints "Struct array nested member access test passed!\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
