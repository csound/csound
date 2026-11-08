<CsTest>
description = "array assignments keep their type across local and global spellings"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
#include "../libassert.orc"

opcode CopyMatrix(values:i [] []):i[][]
  xout values
endop

; Exercise both global spellings, including tabs between bracket pairs.
compact@global:i[][] = init:i[][](2, 3)
spaced@global:i[]	[] = init:i [] [](2, 3)
compact[1][2] = 42
spaced[1][2] = 84

instr 1
  fromCompact:i[][] = CopyMatrix(compact)
  fromSpaced:i[] [] = CopyMatrix(spaced)
  explicit:i [] [] = CopyMatrix:i [] [](compact)
  copy@global:i [] [] = CopyMatrix(spaced)
  assertEquals(fromCompact[1][2], 42)
  assertEquals(fromSpaced[1][2], 84)
  assertEquals(explicit[1][2], 42)
  assertEquals(copy[1][2], 84)

  ; The for-loop lexer also needs to handle typed variables and array calls.
  total:i init 0
  for value:i in fillarray:i[](1, 2, 3) do
    total += value
  od
  for globalValue@global:i in fillarray:i [](4, 5, 6) do
    total += globalValue
  od
  assertEquals(total, 21)
endin

instr 2
  ; Values declared with @global remain visible in another instrument.
  assertEquals(copy[1][2], 84)
  assertEquals(globalValue, 6)
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 2 0.01 0.01
e
</CsScore>
</CsoundSynthesizer>
