<CsoundSynthesizer>
<CsOptions>
--enable-assert
</CsOptions>
<CsInstruments>
sr=44100
ksmps=1
nchnls=1

instr 1
  i1 = 1 + 1
  assert_true(i1 == 2)
  assert_false(i1 == 3)
endin

</CsInstruments>
<CsScore>
i1 0 0
e
</CsScore>
</CsoundSynthesizer>
