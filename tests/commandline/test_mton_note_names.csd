<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  SExpected strget p5
  ; Start with a short existing string to exercise output resizing.
  SInit strcpy "x"
  SInit mton p4
  if strcmp(SInit, SExpected) != 0 then
    printf_i "FAIL mton(%g): %s expected %s\n", 1, p4, SInit, SExpected
    exitnow -1
  endif
  kBlock init 0
  kMidi = (kBlock % 2 == 0 ? p4 : 60)
  SActual mton kMidi
  if kBlock % 2 == 0 then
    kDifferent strcmpk SActual, SExpected
  else
    kDifferent strcmpk SActual, "4C"
  endif
  if kDifferent != 0 then
    printks "FAIL k-rate mton(%g): %s\n", 0, kMidi, SActual
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 20 then
    prints "FAIL mton checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Low MIDI notes, cent carry, spelling, and octaves outside one digit.
i 1 0 .0625 0 "-1C"
i 1 0.125 .0625 1 "-1C#"
i 1 0.25 .0625 1.25 "-1C#+25"
i 1 0.375 .0625 11 "-1B"
i 1 0.5 .0625 11.75 "0C-25"
i 1 0.625 .0625 11.999 "0C"
i 1 0.75 .0625 12 "0C"
i 1 0.875 .0625 60 "4C"
i 1 1 .0625 60.04 "4C+4"
i 1 1.125 .0625 60.4 "4C+40"
i 1 1.25 .0625 60.5 "4C+"
i 1 1.375 .0625 60.75 "4C#-25"
i 1 1.5 .0625 60.999 "4C#"
i 1 1.625 .0625 61.5 "4C#+"
i 1 1.75 .0625 59.999 "4C"
i 1 1.875 .0625 127 "9G"
i 1 2 .0625 132 "10C"
i 1 2.125 .0625 -1 "-2B"
i 1 2.25 .0625 -0.25 "-1C-25"
i 1 2.375 .0625 -12.25 "-2C-25"
i 99 2.5 .015625
e
</CsScore>
</CsoundSynthesizer>
