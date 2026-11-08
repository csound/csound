<CsTest>
description = "fprints and fprintks preserve literals, escapes, conversions and full-length output"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1000
ksmps = 1
nchnls = 1
gSLong init "x"
gSEscapes init ""

instr 1
  iCount = 0
  while iCount < 13 do
    gSLong strcat gSLong, gSLong
    iCount += 1
  od
  fprints "fprints_empty.txt", ""
  SSlash sprintf "%c", 92
  fprints "fprints_slash.txt", SSlash
  fprints "fprints_long.txt", gSLong
  fprints "fprints_fields.txt", "%4096s%4096s", "left", "right"
  fprints "fprints_integers.txt", "%hhd %hd %d %ld %lld | %hhu %hu %u %lu %llu | %o %x %X %c", -3, -3, -3, -3, -3, 3, 3, 3, 3, 3, 8, 31, 31, 65
  fprints "fprints_floats.txt", "%e %E %f %F %g %G %.1a %.1A %lf", 1, 1, 1, 1, 1, 1, 1, 1, 1
  fprints "fprints_escapes.txt", "^^ ~~ ^ ~ %!%tend%r"
  gSEscapes sprintf "^ ~ %c %c[ ;\tend\r", 27, 27
endin

instr 2
  kCycle timeinstk
  fprintks "fprintks_values.txt", "k:%d %% after %s%n", kCycle, "text"
  fprintks "fprintks_empty.txt", ""
  fprintks p4, "value:%d%n", kCycle
  if kCycle == 3 then
    turnoff
  endif
endin

instr 3
  SFile strget p4
  SExpected strget p5
  SActual, iLine readfi SFile
  if strcmp(SActual, SExpected) != 0 then
    prints "%s: got [%s], expected [%s]\n", SFile, SActual, SExpected
    exitnow -1
  endif
endin

instr 4
  ; Supply enough space to read the full 8192-character line at once.
  SActual strcat gSLong, gSLong
  SActual, iLine readfi "fprints_long.txt"
  if strcmp(SActual, gSLong) != 0 then
    prints "fprints did not preserve all 8192 literal characters\n"
    exitnow -1
  endif
  SFields strcat gSLong, gSLong
  SFields, iLine readfi "fprints_fields.txt"
  SLeft strsub SFields, 4092, 4096
  SRight strsub SFields, 8187, 8192
  if strlen(SFields) != 8192 || strcmp(SLeft, "left") != 0 || strcmp(SRight, "right") != 0 then
    prints "fprints truncated or misplaced a formatted field\n"
    exitnow -1
  endif
  SEscapes, iLine readfi "fprints_escapes.txt"
  if strcmp(SEscapes, gSEscapes) != 0 then
    prints "fprints changed the legacy escape codes\n"
    exitnow -1
  endif
  SSlash, iLine readfi "fprints_slash.txt"
  SExpected sprintf "%c", 92
  if strcmp(SSlash, SExpected) != 0 then
    prints "fprints did not preserve a trailing backslash\n"
    exitnow -1
  endif
endin

instr 5
  iCount = 1
read:
  SActual, iLine readfi "fprintks_values.txt"
  SNumeric, iLine readfi "fprintks_numeric.txt"
  SNumericExpected sprintf "value:%d\n", iCount
  SExpected sprintf "k:%d %% after text\n", iCount
  if strcmp(SActual, SExpected) != 0 || strcmp(SNumeric, SNumericExpected) != 0 then
    prints "fprintks cycle %g: got [%s], expected [%s]\n", iCount, SActual, SExpected
    exitnow -1
  endif
  iCount += 1
  if iCount <= 3 igoto read
endin

instr 6
  ; Exercise the numeric signature with a string p-field.
  fprints p4, "before %d %% literal %%n %s %.2f after%r", 7, "text", 1.25
endin
</CsInstruments>
<CsScore>
i1 0 .001
i2 .01 .01 "fprintks_numeric.txt"
i6 .01 .001 "fprints_numeric.txt"
i3 .03 .001 "fprints_empty.txt" ""
i3 .03 .001 "fprintks_empty.txt" ""
i3 .03 .001 "fprints_numeric.txt" "before 7 % literal %n text 1.25 after\r"
i3 .03 .001 "fprints_integers.txt" "-3 -3 -3 -3 -3 | 3 3 3 3 3 | 10 1f 1F A"
i3 .03 .001 "fprints_floats.txt" "1.000000e+00 1.000000E+00 1.000000 1.000000 1 1 0x1.0p+0 0X1.0P+0 1.000000"
i4 .03 .001
i5 .03 .001
</CsScore>
</CsoundSynthesizer>
