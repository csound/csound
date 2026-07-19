<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>

instr 1
  Sarguments:S[] argv

  if (lenarray(Sarguments) != 4) then
    prints "argv length failed: expected 4, got %d\n", lenarray(Sarguments)
    exitnow(1)
  endif

  if (
    strcmp(Sarguments[0], "concert.orc") != 0 ||
    strcmp(Sarguments[1], "first violin") != 0 ||
    strcmp(Sarguments[2], "--logfile=ignored") != 0 ||
    strcmp(Sarguments[3], "") != 0
  ) then
    prints(
      "argv values failed: '%s', '%s', '%s', '%s'\n",
      Sarguments[0], Sarguments[1], Sarguments[2], Sarguments[3]
    )
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
