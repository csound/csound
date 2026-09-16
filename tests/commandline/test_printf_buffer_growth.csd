<CsTest>
description = "Formatted strings retain complete fields and suffixes when buffers grow"

[expect]
exit = 0
stderr_regex = [
  '(?m)^PRINTF:0{4095}7:0{511}9:END\r?$',
  '(?m)^LITERAL:L{4096}:END\r?$',
  '(?m)^PERF:0{511}7:END\r?$'
]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1

instr 1
  SZeros = ""
  SLiteral = "LITERAL:"
  iIndex = 0
build:
  SZeros strcat SZeros, "0"
  SLiteral strcat SLiteral, "L"
  iIndex += 1
  if iIndex < 4096 igoto build
  SLiteral strcat SLiteral, ":END\n"
  printf_i SLiteral, 1
  printf_i "PRINTF:%04096d:%0512d:END\n", 1, 7, 9

  SZero255 strsub SZeros, 0, 255
  SExpected strcat SZero255, "7|"
  SExpected strcat SExpected, SZero255
  SExpected strcat SExpected, "9|END"
  SResult sprintf "%0256d|%0256d|END", 7, 9
  if strcmp(SResult, SExpected) != 0 then
    prints "Formatted integer fields or suffix were lost\n"
    exitnow -1
  endif

  SZero126 strsub SZeros, 0, 126
  SExpected strcat "1.25", SZero126
  SResult sprintf "%.128f", 1.25
  if strcmp(SResult, SExpected) != 0 then
    prints "Formatted precision was truncated\n"
    exitnow -1
  endif
  SResult sprintf "%-512s:END", "word"
  SHead strsub SResult, 0, 4
  STail strsub SResult, 512
  if strlen(SResult) != 516 || strcmp(SHead, "word") != 0 || strcmp(STail, ":END") != 0 then
    prints "Formatted string width or suffix was lost\n"
    exitnow -1
  endif
  SResult sprintf "%+d %i %#o %#x %#X %u %c %.2e %.2E %.2f %.2F %.2g %.2G %%", 2.6, -2.6, 8, 31, 31, 7, 65, 1.25, 1.25, 1.25, 1.25, 1.25, 1.25
  if strcmp(SResult, "+3 -3 010 0x1f 0X1F 7 A 1.25e+00 1.25E+00 1.25 1.25 1.2 1.2 %") != 0 then
    prints "Existing format conversions changed: %s\n", SResult
    exitnow -1
  endif

  kCycle timeinstk
  SFormat = "%d"
  if kCycle == 2 then
    SFormat strcpyk "%0512d:END"
  elseif kCycle >= 3 then
    SFormat strcpyk "%d"
  endif
  SControl sprintfk SFormat, 7
  if kCycle == 2 then
    if strlenk(SControl) != 516 then
      printks "sprintfk did not grow its output\n", 0
      exitnowk -1
    endif
  elseif strcmpk(SControl, "7") != 0 then
    printks "sprintfk retained a stale suffix\n", 0
    exitnowk -1
  endif
  printf "PERF:%0512d:END\n", (kCycle == 2 ? 1 : 0), 7
endin
</CsInstruments>
<CsScore>
i 1 0 0.25
e
</CsScore>
</CsoundSynthesizer>
