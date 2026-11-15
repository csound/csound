<CsTest>
description = "a repeat count in [] adds and subtracts left to right"

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
giExpected[] fillarray 5, 9, 3
giPlayed[] init lenarray(giExpected)

instr Count
  giPlayed[p4] = giPlayed[p4] + 1
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
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
