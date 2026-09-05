<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
ksmps = 1
instr 1
  Stext init "A long string reserves more storage than its next value needs. Shortening it must not make strcat copy that spare capacity."
  Stext = "i"
  Sresult strcat Stext, "[]"
  if strcmp(Sresult, "i[]") != 0 then
    exitnow 1
  endif
  Sright init "Another long string reserves more storage than the first input has. Prepending must read only the input text."
  Sright = "b"
  Sleft = "a"
  Sright strcat Sleft, Sright
  if strcmp(Sright, "ab") != 0 then
    exitnow 1
  endif
  Sleft strcat Sleft, "b"
  Sleft strcat Sleft, Sleft
  if strcmp(Sleft, "abab") != 0 then
    exitnow 1
  endif
  Sempty = ""
  Sempty strcat Sempty, Sempty
  if strlen(Sempty) != 0 then
    exitnow 1
  endif
  prints "strcat buffer capacity passed\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
