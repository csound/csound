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
  Sperf strcpyk 7
  if strcmpk(Sperf, strget(7)) != 0 then
    printks "strcpyk did not give the long strset string\n", 0
    exitnowk -1
  endif
  giChecks += 1
  turnoff
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
  ; A literal passed by reference is the string pool's buffer too, so a text
  ; that fits stays in it.
  AssignNumber("a literal the string pool holds, and much longer than any soundin name it could be given")
  AssignNumber("a literal the string pool holds, and much longer than any soundin name it could be given")
  giChecks += 1
endin

instr 5
  ; printks2 keeps a pointer to its format, so a text that fits stays in it.
  kCycle timeinstk
  printks2 gSformat, kCycle
endin

instr 6
  strset 8, "format after %.0f\n"
  gSformat = 8
  giChecks += 1
endin

instr 99
  if giChecks != 6 then
    prints "string checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1 7
i 2 .1 .1
i 3 .2 .1 3
i 4 .3 .1
i 4 .4 .1
i 5 .5 .25
i 6 .6 .01
i 99 .8 .01
e
</CsScore>
</CsoundSynthesizer>
