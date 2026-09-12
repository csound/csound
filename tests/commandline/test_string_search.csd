<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1

opcode CheckSearch, 0, SSii
  Stext, Sfind, iFirstExpected, iLastExpected xin
  iFirst strindex Stext, Sfind
  iLast strrindex Stext, Sfind
  if iFirst != iFirstExpected || iLast != iLastExpected then
    prints "i-rate search failed: '%s' in '%s': %g, %g\n", Sfind, Stext, iFirst, iLast
    exitnow -1
  endif
  kFirst strindexk Stext, Sfind
  kLast strrindexk Stext, Sfind
  if kFirst != iFirstExpected || kLast != iLastExpected then
    printks "k-rate search failed: '%s' in '%s': %g, %g\n", 0, Sfind, Stext, kFirst, kLast
    exitnowk -1
  endif
endop

instr 1
  CheckSearch "aaab", "aab", 1, 1
  CheckSearch "aaa", "aa", 0, 1
  CheckSearch "ababa", "aba", 0, 2
  CheckSearch "abababac", "ababac", 2, 2
  CheckSearch "aaaaab", "aaab", 2, 2
  CheckSearch "abababcabababc", "ababc", 2, 9
  CheckSearch "abcabc", "abc", 0, 3
  CheckSearch "abc", "abc", 0, 0
  CheckSearch "abc", "a", 0, 0
  CheckSearch "abc", "c", 2, 2
  CheckSearch "aaa", "a", 0, 2
  CheckSearch "abc", "d", -1, -1
  CheckSearch "abc", "abcd", -1, -1
  CheckSearch "abcab", "abcabc", -1, -1
  CheckSearch "abc", "", 0, 3
  CheckSearch "", "abc", -1, -1
  CheckSearch "", "", 0, 0
  ; Positions count bytes, including in UTF-8 strings.
  CheckSearch "ééé", "éé", 0, 2
endin

instr 2
  kCycle init 0
  kCycle += 1
  if kCycle == 1 then
    Stext strcpyk "aaab"
    Sfind strcpyk "aab"
    kFirstExpected = 1
    kLastExpected = 1
  elseif kCycle == 2 then
    Stext strcpyk "ababa"
    Sfind strcpyk "aba"
    kFirstExpected = 0
    kLastExpected = 2
  elseif kCycle == 3 then
    Sfind strcpyk ""
    kFirstExpected = 0
    kLastExpected = 5
  else
    Stext strcpyk ""
    Sfind strcpyk "x"
    kFirstExpected = -1
    kLastExpected = -1
  endif
  kFirst strindexk Stext, Sfind
  kLast strrindexk Stext, Sfind
  if kFirst != kFirstExpected || kLast != kLastExpected then
    printks "Changing string search failed at cycle %g: %g, %g\n", 0, kCycle, kFirst, kLast
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0625
i 2 0 .0625
e
</CsScore>
</CsoundSynthesizer>
