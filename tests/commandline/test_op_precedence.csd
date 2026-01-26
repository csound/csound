<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>

; Helper opcode for assertions
opcode assert, 0, iS
  ival, Smsg xin
  if (ival == 0) then
    prints "ASSERTION FAILED: %s\n", Smsg
    exitnow 1
  endif
endop

instr 1
  ; --- 1. Bitwise vs Equality/Relational ---
  ; EXPECTED (csound6): use explicit parentheses to avoid invalid boolean bitwise ops
  ires = (1 & 0) == 0
  assert( ires, "1 & 0 == 0")
  ires = (0 | 1) == 1
  assert( ires, "0 | 1 == 1")
  ires = (1 # 1) == 0
  assert( ires, "1 # 1 == 0")

  ; EXPECTED: (1 & 1) > 0 -> (1) > 0 -> TRUE
  ires = (1 & 1) > 0
  assert( ires, "1 & 1 > 0")

  ; --- 2. Bitshifts vs Bitwise ---
  ; EXPECTED: (1 << 1) & 2 -> 2 & 2 -> 2
  ires = 1 << 1 & 2
  iok = (ires == 2)
  assert( iok, "1 << 1 & 2 == 2")

  ; EXPECTED: 2 >> (1 & 1) -> 2 >> 1 -> 1
  ires = 2 >> 1 & 1
  iok = (ires == 1)
  assert( iok, "2 >> 1 & 1 == 1")

  ; --- 3. Additive vs Bitwise/Shift ---
  ; EXPECTED: (1 + 1) << 1 -> 2 << 1 -> 4
  ires = 1 + 1 << 1
  iok = (ires == 4)
  assert( iok, "1 + 1 << 1 == 4")

  ; EXPECTED: (1 + 1) & 2 -> 2 & 2 -> 2
  ires = 1 + 1 & 2
  iok = (ires == 2)
  assert( iok, "1 + 1 & 2 == 2")

  ; --- 4. Multiplicative vs Additive ---
  ; EXPECTED: 1 + (2 * 3) -> 1 + 6 -> 7
  ires = 1 + 2 * 3
  iok = (ires == 7)
  assert( iok, "1 + 2 * 3 == 7")

  ; --- 5. Logical vs Bitwise ---
  ; EXPECTED: (1 & 0 != 0) || (1 != 0) -> (FALSE) || (TRUE) -> TRUE
  ; If || was higher precedence than &: 1 & (0 || 1) -> 1 & 1
  ires = ((1 & 0) != 0) || (1 != 0)
  assert( ires, "1 & 0 != 0 || 1 != 0")

  ; --- 6. Unary vs others ---
  ires = -1 + 2
  iok = (ires == 1)
  assert( iok, "-1 + 2 == 1")
  ires = (~0 & 1) == 1
  assert( ires, "~0 & 1 == 1")

  prints "ALL PRECEDENCE TESTS PASSED\n"
  exitnow 0
endin

</CsInstruments>
<CsScore>
i 1 0 1
</CsScore>
</CsoundSynthesizer>
