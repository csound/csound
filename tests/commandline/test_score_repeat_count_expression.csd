<CsTest>
description = "a repeat count in [] adds and subtracts left to right, and reads on after a [] inside it, keeps its counter, and keeps a minus sign"

[expect]
exit = 0
output = ["every repeat count was right"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

; Each repeat plays Count with its own number as p4. A + or - took only
; the operator above it: [10-2*3+1] was 10-(6+1), and played 3 times.
; And a [] inside the count lost the character after it: [[2]*3] was 3.
; A minus sign before a number was dropped: [-2+5] was 7.
giExpected[] fillarray 5, 9, 3, 6, 7, 3, 3, 3, 3
giPlayed[] init lenarray(giExpected)
; And a count in [] lost the name of its counter, which was then undefined.
giCounters init 0

instr Count
  giPlayed[p4] = giPlayed[p4] + 1
endin

instr Counter
  giCounters += p4
endin

instr Check
  wrong:i = 0
  index:i = 0
  while index < lenarray(giExpected) do
    if giPlayed[index] != giExpected[index] then
      prints("repeat %d played %d times, not %d\n", index, giPlayed[index], giExpected[index])
      wrong += 1
    endif
    index += 1
  od
  if giCounters != 6 then
    prints("the counters of repeats counted in [] added up to %d, not 6\n", giCounters)
    wrong += 1
  endif
  if wrong != 0 then
    exitnow(1)
  endif
  prints("every repeat count was right\n")
endin
</CsInstruments>
<CsScore>
{ [10-2*3+1] A
i "Count" 0 0 0
}
{ [10-6/3+1] B
i "Count" 0 0 1
}
{ [2*5-4*2+1] C
i "Count" 0 0 2
}
{ [[2]*3] D
i "Count" 0 0 3
}
{ [1+[2]*3] E
i "Count" 0 0 4
}
{ [3] F
i "Count" 0 0 5
i "Counter" 0 0 $F.
}
{ [[1]+2] G
i "Counter" 0 0 $G.
}
{ [-2+5] H
i "Count" 0 0 6
}
{ [10*-1+13] I
i "Count" 0 0 7
}
#define MINUS_ONE #-1#
{ [4+$MINUS_ONE.] J
i "Count" 0 0 8
}
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
