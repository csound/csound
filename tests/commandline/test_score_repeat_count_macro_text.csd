<CsTest>
description = "a repeat count from a macro whose text calls a macro or holds a [] is read from that text expanded"

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

; Each repeat plays Count with its own number as p4, and Counter with its
; counter. The count of a macro without arguments was the number at the
; start of its text, unexpanded: a text that calls another macro, or holds
; a [], gave 0, and the score stopped with "{: invalid repeat count".
; What follows the count in the text is ignored, as it was: the counter is
; the name after the call, after a { that ends a macro's text too. And as
; strtoll took them, the text may begin with line ends and a plus sign.
giExpected[] fillarray 3, 4, 2, 2, 2, 2, 3
giPlayed[] init lenarray(giExpected)
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
  ; 0+1+2, 0+1+2+3, 0+1, 0+1, 0+1, 0+1 and 0+1+2
  if giCounters != 16 then
    prints("the counters added up to %d, not 16\n", giCounters)
    wrong += 1
  endif
  if wrong != 0 then
    exitnow(1)
  endif
  prints("every repeat count was right\n")
endin
</CsInstruments>
<CsScore>
#define THREE #3#
#define ALIAS #$THREE#
#define EXPR #[2*2]#
#define ARG(x) #$x#
#define TWO_X #2 X#
#define OPEN #{#
#define LINES #
2
#
#define PLUS #+3#
{ $ALIAS A
i "Count" 0 0 0
i "Counter" 0 0 $A
}
{ $EXPR. B
i "Count" 0 0 1
i "Counter" 0 0 $B
}
{ $ARG([1+1]) C
i "Count" 0 0 2
i "Counter" 0 0 $C
}
{ $TWO_X D
i "Count" 0 0 3
i "Counter" 0 0 $D
}
$OPEN $TWO_X E
i "Count" 0 0 4
i "Counter" 0 0 $E
}
{ $LINES F
i "Count" 0 0 5
i "Counter" 0 0 $F
}
{ $PLUS G
i "Count" 0 0 6
i "Counter" 0 0 $G
}
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
