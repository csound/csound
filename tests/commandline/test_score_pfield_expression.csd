<CsTest>
description = "a p-field's [] adds and subtracts left to right, and takes ^ first"

[expect]
exit = 0
output = ["every p-field expression was right"]
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

; Each note gives an expression in p4 and what it must come to in p5. A +
; or - took only the operator above it: [10-2*3+1] was 10-(6+1) = 3. And
; an operator after a ^ went first: [2^3*2] was 2^6.
giWrong init 0

instr Value
  if p4 != p5 then
    prints("expression %d gave %g, not %g\n", p6, p4, p5)
    giWrong += 1
  endif
endin

instr Check
  if giWrong != 0 then
    exitnow(1)
  endif
  prints("every p-field expression was right\n")
endin
</CsInstruments>
<CsScore>
i "Value" 0 0 [10-2*3+1]     5   0
i "Value" 0 0 [10-6/3+1]     9   1
i "Value" 0 0 [2*3-4*5+6]   -8   2
i "Value" 0 0 [10-0.5*3+1]   9.5 3
i "Value" 0 0 [10-2^3+1]     3   4
i "Value" 0 0 [4+3*2+1]     11   5
i "Value" 0 0 [4+3-2+1]      6   6
i "Value" 0 0 [2^3*2]        16   7
i "Value" 0 0 [2^3/2]         4   8
i "Value" 0 0 [2^3%3]         2   9
i "Value" 0 0 [2^3&1]         0  10
i "Value" 0 0 [2^3|1]         9  11
i "Value" 0 0 [2^3#1]         9  12
i "Value" 0 0 [2*3^2/2^3]     2.25 13
i "Value" 0 0 [2*3^2]        18  14
i "Value" 0 0 [2^3^2]       512  15
i "Check" 0.1 0
</CsScore>
</CsoundSynthesizer>
