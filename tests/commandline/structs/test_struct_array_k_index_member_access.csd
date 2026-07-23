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

; Guards perf-time struct-array member reads: array[kIndex].member must lower
; to the k-rate ##array_get_struct path instead of failing opcode resolution.
struct MyType val1:i, val2:i

instr 1
  array:MyType[] init 2
  array[0].val1 = 1
  array[1].val1 = 2

  ; An explicit aggregate init must read its k-indexed source at init.
  copyIndex:k init 1
  copy:MyType init array[copyIndex]
  assertEquals(copy.val1, 2)

  ; Switch the index at k-rate and read the selected member each control pass.
  kindex = int(timeinstk() / 5)
  kval = array[kindex].val1

  kchanges init 0
  ktrigger changed kval

  ; Assert the observed value matches the active array element.
  if (ktrigger == 1) then
    kchanges += 1
    event "i", 2, 0, 0.001, kval, kindex + 1
  endif

  ; We expect two observed values: 1 first, then 2 after the index changes.
  if (timeinstk() == 8) then
    event "i", 2, 0, 0.001, kchanges, 2
    turnoff
  endif
endin

instr 2
  assertEquals(p4, p5)
endin

</CsInstruments>
<CsScore>
i 1 0 0.1
</CsScore>
</CsoundSynthesizer>
