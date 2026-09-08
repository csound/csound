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
struct Selection idx:k

opcode PickAt(items:MyType[], index:k):MyType
  xout items[index]
endop

opcode CheckInitValue(item:MyType):void
  assertEquals(item.val1, 2)
endop

opcode CheckPerfValue(item:MyType):void
  if (timeinstk() == 1) then
    event "i", 2, 0, 0.001, item.val1, 2
  endif
endop

instr 1
  array:MyType[] init 2
  array[0].val1 = 1
  array[1].val1 = 2

  ; Literal and i-rate indices select init-capable struct-array reads.
  literalCopy:MyType init array[0]
  assertEquals(literalCopy.val1, 1)

  copyIndex:i init 1
  copy:MyType init array[copyIndex]
  assertEquals(copy.val1, 2)

  ; K-rate indices require an explicit conversion for an init-time read.
  convertedIndex:k init 1
  convertedCopy:MyType init array[i(convertedIndex)]
  assertEquals(convertedCopy.val1, 2)

  ; Init-only UDOs accept values read at init, including converted indices.
  CheckInitValue array[1]
  CheckInitValue array[copyIndex]
  CheckInitValue array[i(convertedIndex)]

  selection:Selection init 1
  memberCopy:MyType init array[i(selection.idx)]
  assertEquals(memberCopy.val1, 2)

  indices:k[] fillarray 1
  indexedCopy:MyType init array[i(indices, 0)]
  assertEquals(indexedCopy.val1, 2)

  ; A performance read must not evaluate an invalid k-index during init.
  perfIndex:k init -1
  perfIndex = 0
  perfValue:k = array[perfIndex].val1

  ; An opcode with init setup and a perf callback must wait for perf to read.
  printIndex:k init -1
  printIndex = 0
  printks "PERF_MEMBER=%f\n", 0.1, array[printIndex].val1

  ; Init-only type inspection must not force the getter to read at init.
  typeIndex:k init -1
  typeIndex = 0
  printtype array[typeIndex]

  ; Nested type inspection must not force its operand to run at init.
  nestedTypeIndex:k init -1
  nestedTypeIndex = 0
  prints "NESTED_TYPE=%s\n", typeof(array[nestedTypeIndex + 0].val1)

  ; The same rule applies to an aggregate read lowered inside xout.
  udoIndex:k init -1
  udoIndex = 1
  udoValue:MyType = PickAt(array, udoIndex)
  CheckPerfValue array[udoIndex]

  ; Switch the index at k-rate and read the selected member each control pass.
  kindex = int(timeinstk() / 5)
  kval = array[kindex].val1

  kchanges init 0
  ktrigger changed kval

  ; Assert the observed value matches the active array element.
  if (timeinstk() == 1) then
    event "i", 2, 0, 0.001, perfValue, 1
    event "i", 2, 0, 0.001, udoValue.val1, 2
  endif
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
