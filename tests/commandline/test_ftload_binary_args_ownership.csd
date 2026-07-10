<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

giSrc = ftgen(1, 0, 16, -2, 11, 22, 33, 44)
giDst = ftgen(2, 0, 16, 2, 0)

opcode AssertTableValue(ndx:i, fn:i, exptc:i):void
  value:i = table:i(ndx, fn)
  if value != exptc then
    prints("ftload ownership: value mismatch at index %d, expected %d, got %f\n", ndx, exptc, value)
    exitnow(-1)
  endif
endop

instr 1
  prints("ftload ownership: saving table 1\n")
  ftsave("test_ftload_binary_args_ownership.ftsave", 0, 1)
  prints("ftload ownership: loading table 1 as table 2\n")
  ftload("test_ftload_binary_args_ownership.ftsave", 0, 2)

  prints("ftload ownership: checking loaded table data\n")
  AssertTableValue(0, 2, 11)
  AssertTableValue(1, 2, 22)
  AssertTableValue(2, 2, 33)
  AssertTableValue(3, 2, 44)

  prints("ftload ownership: freeing table 2\n")
  ftfree(2, 0)
  prints("ftload ownership: freeing table 1\n")
  ftfree(1, 0)
  prints("ftload ownership: passed\n")
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
