<CsTest>
description = "braces, and n or m before a space, are text inside a quoted score string, and braces outside one are still a repeat"

[expect]
exit = 0
output = ["every score string arrived whole"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
; Read inside a string, a brace was the score's repeat: "{1,2,3}" took the
; rest of the line and every line after it, and "{kick,snare}" and "a}b"
; stopped the score. And n or m and a space began a mark's name: the number
; after "gain " was lost, and the closing quote after "drum 60" with it,
; which took the next line into the string.
gSexpected[] fillarray "/kit/tala/{1,2,3}/0/drive", "{kick,snare}", "a}b", \
                       "gain 1 bounded", "main 2nd=1", "drum 60"
giSeen init 0
giLoops init 0

instr Show
  given:S = p4
  index:i = p5
  if strcmp(given, gSexpected[index]) != 0 then
    prints("score string %d arrived as [%s], not [%s]\n", index, given, gSexpected[index])
    exitnow(1)
  endif
  if p6 != index + 10 then
    prints("the field after score string %d arrived as %g\n", index, p6)
    exitnow(1)
  endif
  giSeen += 1
endin

instr Loop
  giLoops += 1
endin

instr Check
  if giSeen != 6 || giLoops != 2 then
    prints("%d of 6 score strings and %d of 2 repeats arrived\n", giSeen, giLoops)
    exitnow(1)
  endif
  prints("every score string arrived whole\n")
endin
</CsInstruments>
<CsScore>
i "Show" 0 0.01 "/kit/tala/{1,2,3}/0/drive" 0 10
i "Show" 0 0.01 "{kick,snare}" 1 11
i "Show" 0 0.01 "a}b" 2 12
i "Show" 0 0.01 "gain 1 bounded" 3 13
i "Show" 0 0.01 "main 2nd=1" 4 14
i "Show" 0 0.01 "drum 60" 5 15
{ 2 CNT
i "Loop" 0 0.01
}
i "Check" 0.1 0.01
</CsScore>
</CsoundSynthesizer>
