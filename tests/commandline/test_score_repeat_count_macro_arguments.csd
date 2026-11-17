<CsTest>
description = "a repeat count from a macro with arguments is read from the macro's text, and the counter named after the call is kept"

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
; counter. A count from a macro with arguments put the ( back before reading
; them: the arguments were read to the end of the score, which stopped with
; "unexpected EOF!!". And at the end of the macro's text, the counter's name
; after the call was out of reach.
giExpected[] fillarray 3, 4, 5, 2
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
  ; 0+1+2, 0+1+2+3, 0+1+2+3+4 and 0+1
  if giCounters != 20 then
    prints("the counters added up to %d, not 20\n", giCounters)
    wrong += 1
  endif
  if wrong != 0 then
    exitnow(1)
  endif
  prints("every repeat count was right\n")
endin
</CsInstruments>
<CsScore>
#define THREE(x) #3#
#define ARG(x) #$x#
#define SUM(x'y) #[$x+$y]#
#define TWO(x) #2 #
{ $THREE(1) A
i "Count" 0 0 0
i "Counter" 0 0 $A
}
{ $ARG(4) B
i "Count" 0 0 1
i "Counter" 0 0 $B
}
{ $SUM(2'3) C
i "Count" 0 0 2
i "Counter" 0 0 $C
}
{ $TWO.(0) D
i "Count" 0 0 3
i "Counter" 0 0 $D
}
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
