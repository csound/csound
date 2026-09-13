<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  ; Exercise full, empty, forward and reversed results at one opcode instance.
  kStarts[] fillarray 0, 0, 1, 2, 3, 9, -1, 0
  kEnds[] fillarray 3, 0, 3, 2, 0, 8, 0, 3
  SExpected[] fillarray "abc", "", "bc", "", "cba", "", "cba", "abc"
  kCycle init 0
  SPart strsubk "abc", kStarts[kCycle], kEnds[kCycle]
  SCopy = SPart
  SCopy2 = SCopy
  kPart strcmpk SPart, SExpected[kCycle]
  kCopy strcmpk SCopy, SExpected[kCycle]
  kCopy2 strcmpk SCopy2, SExpected[kCycle]
  if kPart != 0 || kCopy != 0 || kCopy2 != 0 then
    printks "strsubk assignment failed at cycle %g: '%s', '%s', '%s'\n", 0, kCycle, SPart, SCopy, SCopy2
    exitnowk -1
  endif
  kCycle += 1
endin

instr 2
  ; Reusing the source must still publish empty and reversed results.
  kCycle init 0
  SText strcpyk "abc"
  kEnd = (kCycle % 2 == 0 ? 0 : 3)
  SText strsubk SText, 3, kEnd
  SCopy = SText
  SExpected strcpyk "cba"
  if kCycle % 2 != 0 then
    SExpected strcpyk ""
  endif
  kText strcmpk SText, SExpected
  kCopy strcmpk SCopy, SExpected
  if kText != 0 || kCopy != 0 then
    printks "strsubk source reuse failed at cycle %g\n", 0, kCycle
    exitnowk -1
  endif
  kCycle += 1
endin

instr 3
  ; Init-time behavior is unchanged, including empty input and defaults.
  SEmpty strsub "abc", 1, 1
  SFromEmpty strsub "", 0, -1
  SDefault strsub "abc"
  SReverse strsub "abc", -1, 0
  if strlen(SEmpty) != 0 || strlen(SFromEmpty) != 0 || \
      strcmp(SDefault, "abc") != 0 || strcmp(SReverse, "cba") != 0 then
    prints "strsub init result failed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .125
i 2 0 .125
i 3 0 0
e
</CsScore>
</CsoundSynthesizer>
