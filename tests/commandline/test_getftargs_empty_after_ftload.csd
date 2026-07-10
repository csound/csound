<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

giSrc = ftgen(1, 0, 16, -2, 1)
giDst = ftgen(2, 0, 16, 2, 0)

instr 1
  ftsave("test_getftargs_empty_after_ftload.ftsave", 0, 1)
  ftload("test_getftargs_empty_after_ftload.ftsave", 0, 2)
  Sargs getftargs 2, 1

  if strlen(Sargs) != 0 then
    prints("getftargs: expected empty args after ftload, got '%s'\n", Sargs)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
