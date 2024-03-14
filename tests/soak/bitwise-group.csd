<CsoundSynthesizer>
<CsOptions>
--nosound
</CsOptions>
<CsInstruments>

sr     =    44100
ksmps  =    32
nchnls =    2
0dbfs  =    1

; By Daniele Cucchi - 2020

;a & b: bitwise AND
;a | b: bitwise OR
;a # b: bitwise NON EQUIVALENCE - XOR
; ~ a: bitwise NOT

instr 1

kArrayA[] fillarray 0, 0, 1, 1  ; Fill array "A" with 4 values: 0, 0, 1, 1
i0A = i(kArrayA, 0)
i1A = i(kArrayA, 1)
i2A = i(kArrayA, 2)
i3A = i(kArrayA, 3)

kArrayB[] fillarray 0, 1, 0, 1  ; Fill array "B" with 4 values: 0, 1, 0, 1
i0B = i(kArrayB, 0)
i1B = i(kArrayB, 1)
i2B = i(kArrayB, 2)
i3B = i(kArrayB, 3)

; Bitwise operations & fill arrays
kAND[] = kArrayA & kArrayB
kOR[] = kArrayA | kArrayB
kNON[] fillarray i0B # i0A, i1B # i1A, i2B # i2A, i3B # i3A
kNOT[] fillarray  ~ i0A, ~ i1A, ~ i2A, ~ i3A

; Print values 
printarray kAND, "%d", "= bitwise AND"
printarray kOR, "%d", "= bitwise OR"
printarray kNON, "%d", "= bitwise NON"
printarray kNOT, "%d", "= bitwise NOT\n"

endin

</CsInstruments>
<CsScore>
i1 0 1
e
</CsScore>
</CsoundSynthesizer>
