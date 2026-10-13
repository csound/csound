<CsTest>
description = "readfi restarts on each UDO call and retains EOF within a call"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1

opcode ReadFile(path:S, fileNumber:i, attempts:i):S
  text:S init ""
  for attempt in [1 ... attempts] do
    line:S, lineNumber:i readfi path
    numberedLine:S, numberedLineNumber:i readfi fileNumber
    if (strcmp(line, numberedLine) != 0 || lineNumber != numberedLineNumber) then
      prints "String and numeric file arguments returned different results\n"
      exitnow(1)
    endif
    text strcat text, line
  od
  xout text
endop

instr 1
  ; Read to EOF twice, stop early, start again, then change files.
  paths:S[] fillarray "readf_lines.txt", "readf_lines.txt", "readf_lines.txt", \
                      "readf_lines.txt", "readf_empty.txt", "readf_other.txt", \
                      "readf_lines.txt"
  attempts:i[] fillarray 5, 5, 1, 5, 5, 5, 5
  expected:S[] fillarray "first\nsecond\n", "first\nsecond\n", "first\n", \
                         "first\nsecond\n", "", "other\n", "first\nsecond\n"
  for index in [0 ... 6] do
    strset 9177, paths[index]
    actual:S = ReadFile(paths[index], 9177, attempts[index])
    if (strcmp(actual, expected[index]) != 0) then
      prints "Read %d of %s: expected <%s>, got <%s>\n", \
             index + 1, paths[index], expected[index], actual
      exitnow(1)
    endif
  od
endin
</CsInstruments>
<CsScore>
i1 0 0
</CsScore>
</CsoundSynthesizer>
