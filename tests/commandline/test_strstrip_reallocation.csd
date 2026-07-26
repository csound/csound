<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>

instr 1
  Sunit = "hello world with lots of leading and trailing spaces to force a realloc"
  Scontent2 strcat Sunit, Sunit
  Scontent4 strcat Scontent2, Scontent2
  Scontent strcat Scontent4, Scontent4
  Sin sprintf "        %s        ", Scontent

  Sboth strstrip Sin
  Sleft strstrip Sin, "l"
  Sright strstrip Sin, "r"

  SleftExpected strcat Scontent, "        "
  SrightExpected strcat "        ", Scontent

  if (strcmp(Sboth, Scontent) != 0) then
    prints "strstrip failed: expected '[%s]', got '[%s]'\n", Scontent, Sboth
    exitnow(1)
  endif
  if (strcmp(Sleft, SleftExpected) != 0) then
    prints "strstrip left failed: expected '[%s]', got '[%s]'\n", \
      SleftExpected, Sleft
    exitnow(1)
  endif
  if (strcmp(Sright, SrightExpected) != 0) then
    prints "strstrip right failed: expected '[%s]', got '[%s]'\n", \
      SrightExpected, Sright
    exitnow(1)
  endif

  SemptyBoth strstrip "        "
  SemptyLeft strstrip "        ", "l"
  SemptyRight strstrip "        ", "r"
  if (
    strcmp(SemptyBoth, "") != 0 ||
    strcmp(SemptyLeft, "") != 0 ||
    strcmp(SemptyRight, "") != 0
  ) then
    prints "strstrip whitespace-only input did not return empty strings\n"
    exitnow(1)
  endif
endin

</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
