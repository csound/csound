<CsTest>
description = "Deprecated tb slots keep global setup and later assignments across instruments"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1

giFirst ftgen 1, 0, 4, -2, 10, 11, 12, 13
giSecond ftgen 2, 0, 4, -2, 20, 21, 22, 23
giLast ftgen 3, 0, 4, -2, 150, 151, 152, 153
; Slots set at global scope must be available in every instrument.
; Both initialization spellings remain supported.
tb0init giFirst
tb15_init giLast
gkBase init 10
gkCycles init 0
giInitChecks init 0

instr Watch
  kIndex init 0
  kFirst = tb0(kIndex)
  kLast = tb15(kIndex)
  ; Watch stays active while other notes reassign slot 0. Slot 15 must
  ; retain its own table, and every element must follow the new assignment.
  if kFirst != gkBase + kIndex || kLast != 150 + kIndex then
    printks "Global tb slots at index %g returned %g, %g instead of %g, %g\n", 0, kIndex, kFirst, kLast, gkBase + kIndex, 150 + kIndex
    exitnowk -1
  endif
  kIndex = (kIndex + 1) % 4
  gkCycles += 1
endin

instr Reassign
  if p4 == 2 then
    tb0_init giSecond
  else
    tb0init giFirst
  endif
  gkBase init p5
endin

instr ReadAtInit
  ; The assigning note has already ended. Its table must remain available
  ; to a new note, including when this instrument reuses an old instance.
  iFirst = tb0(2)
  iLast = tb15(2)
  if iFirst != p4 || iLast != 152 then
    prints "An init-time reader lost the global table assignment\n"
    exitnow -1
  endif
  giInitChecks += 1
endin

instr CheckCompletion
  if i(gkCycles) != 64 || giInitChecks != 3 then
    prints "The global tb checks did not finish\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "Watch" 0 .125
i "Reassign" .03125 0 2 20
i "ReadAtInit" .046875 0 22
i "Reassign" .0625 0 1 10
i "ReadAtInit" .078125 0 12
i "Reassign" .09375 0 2 20
i "ReadAtInit" .109375 0 22
i "CheckCompletion" .15625 0
e
</CsScore>
</CsoundSynthesizer>
