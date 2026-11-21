<CsTest>
description = "A number given to a string variable is its strset string, of any length, or soundin.N"
[expect]
exit = 0
output = ["format after"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1
giChecks init 0
gkPerformed init 0
gSformat init "format before %.0f\n"

opcode AssignNumber(Sname:S):void
  Sname = 3
  if strcmp(Sname, "soundin.3") != 0 then
    prints "a number was not soundin.3 by reference\n"
    exitnow -1
  endif
endop

instr 1
  ; Far past the 64 bytes a string variable starts with.
  Slong = ""
  iCount = 0
  while iCount < 1000 do
    Slong strcat Slong, "0123456789"
    iCount += 1
  od
  strset 7, Slong

  Sinit init 7
  Sassigned = 7
  Scopied strcpy 7
  Spfield = p4
  if strcmp(Sinit, Slong) != 0 || strcmp(Sassigned, Slong) != 0 || \
     strcmp(Scopied, Slong) != 0 || strcmp(Spfield, Slong) != 0 then
    prints "a number did not give its long strset string\n"
    exitnow -1
  endif
  giChecks += 1
endin

instr 2
  ; strcpyk follows a number that changes during performance: a short
  ; strset string, the long one, which outgrows the buffer, an empty one,
  ; then a number with no strset string.
  strset 9, "short"
  strset 10, ""
  Slong strget 7
  kCycle init 0
  kCycle += 1
  Sperf strcpyk (kCycle == 1 ? 9 : (kCycle == 2 ? 7 : (kCycle == 3 ? 10 : 3)))
  if kCycle == 1 && strcmpk(Sperf, "short") != 0 then
    printks "strcpyk did not give the short strset string\n", 0
    exitnowk -1
  elseif kCycle == 2 && strcmpk(Sperf, Slong) != 0 then
    printks "strcpyk did not give the long strset string\n", 0
    exitnowk -1
  elseif kCycle == 3 && strcmpk(Sperf, "") != 0 then
    printks "strcpyk did not give the empty strset string\n", 0
    exitnowk -1
  elseif kCycle == 4 then
    if strcmpk(Sperf, "soundin.3") != 0 then
      printks "strcpyk did not give soundin.3\n", 0
      exitnowk -1
    endif
    gkPerformed = 1
    turnoff
  endif
endin

instr 3
  ; With no strset string a number stays the numbered file name it has
  ; always been, for an opcode taking a file name.
  Sinit init 1 + 2
  Sassigned = 3
  Scopied strcpy 3
  Spfield = p4
  if strcmp(Sinit, "soundin.3") != 0 || strcmp(Sassigned, "soundin.3") != 0 || \
     strcmp(Scopied, "soundin.3") != 0 || strcmp(Spfield, "soundin.3") != 0 then
    prints "a number was not soundin.3\n"
    exitnow -1
  endif
  giChecks += 1
endin

instr 4
  ; A literal passed by reference stays whatever its argument is given.
  AssignNumber("a literal the string pool holds, and much longer than any soundin name it could be given")
  AssignNumber("a literal the string pool holds, and much longer than any soundin name it could be given")
  giChecks += 1
endin

instr 5
  ; printks2 follows its format's variable.
  kCycle timeinstk
  printks2 gSformat, kCycle
endin

instr 6
  strset 8, "format after %.0f\n"
  gSformat = 8
  giChecks += 1
endin

instr 99
  if giChecks != 5 || i(gkPerformed) != 1 then
    prints "string checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1 7
i 2 .1 .3
i 3 .2 .1 3
i 4 .3 .1
i 4 .4 .1
i 5 .5 .25
i 6 .6 .01
i 99 .8 .01
e
</CsScore>
</CsoundSynthesizer>
