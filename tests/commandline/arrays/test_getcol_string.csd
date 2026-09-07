<CsoundSynthesizer>
<CsOptions>
-ndm0
</CsOptions>
<CsInstruments>
instr 1
  src:S[][] init 2, 3
  src fillarray "a", "b", "c", "d", "e", "f"
  col:S[] getcol src, 2
  prints "column 2: %s %s\n", col[0], col[1]
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>

Prints:
column 2: c f
