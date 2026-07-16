<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>

opcode AssertString(actual:S, prefix:S, suffix:S):i
  space:S = sprintf("%c", 32)
  expected:S = prefix
  expected strcat expected, space
  expected strcat expected, suffix
  result:i = 1

  if (strcmp(actual, expected) != 0) then
    prints "String whitespace failed: expected '%s', got '%s'\n", \
      expected, actual
    result = 0
    exitnow(1)
  endif

  xout result
endop

instr 1
  result:i = AssertString("a (next)", "a", "(next)")
  result = AssertString("i (next)", "i", "(next)")
  result = AssertString("k (next)", "k", "(next)")
  result = AssertString("int (3.5)", "int", "(3.5)")
  result = AssertString("sin (phase)", "sin", "(phase)")

  rounded:i = int (3.5)
  if (rounded != 3) then
    prints "Spaced function call failed: expected 3, got %d\n", rounded
    exitnow(1)
  endif

  prints "String function-like whitespace test passed\n"
endin

</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
