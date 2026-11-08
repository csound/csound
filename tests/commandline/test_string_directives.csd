<CsTest>
description = "#exit, #loop and #includestr are text inside a quoted string, in the orchestra and in the score"

[expect]
exit = 0
output = ["every string with a directive arrived whole"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
; Read inside a string, #exit ended the orchestra or the score there,
; #loop - the marker a repeat's body ends with - popped the buffer it was
; read in: the score stopped, and sent from a host through csoundEventString
; it segfaulted. #includestr in a score's string was written into the
; orchestra's buffer, freed by then: a segfault.
gSexpected[] fillarray "/pad/#exit/0/0/out", "#loop", "a #exit b #loop c", "a #includestr b"
giSeen init 0
giLooped init 0

instr Show
  given:S = p4
  index:i = p5
  if strcmp(given, gSexpected[index]) != 0 then
    prints("string %d arrived as [%s], not [%s]\n", index, given, gSexpected[index])
    exitnow(1)
  endif
  if p6 != index + 10 then
    prints("the field after string %d arrived as %g\n", index, p6)
    exitnow(1)
  endif
  giSeen += 1
endin

instr Looped
  given:S = p4
  if strcmp(given, "#exit #loop") != 0 then
    prints("a string in a repeat arrived as [%s]\n", given)
    exitnow(1)
  endif
  giLooped += 1
endin

instr Check
  if giSeen != 4 || giLooped != 2 then
    prints("%d of 4 strings and %d of 2 repeats arrived\n", giSeen, giLooped)
    exitnow(1)
  endif
  prints("every string with a directive arrived whole\n")
endin
</CsInstruments>
<CsScore>
i "Show" 0 0.01 "/pad/#exit/0/0/out" 0 10
i "Show" 0 0.01 "#loop" 1 11
i "Show" 0 0.01 "a #exit b #loop c" 2 12
i "Show" 0 0.01 "a #includestr b" 3 13
{ 2 CNT
i "Looped" 0 0.01 "#exit #loop"
}
i "Check" 0.1 0.01
</CsScore>
</CsoundSynthesizer>
