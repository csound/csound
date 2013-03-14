<CsoundSynthesizer>
<CsInstruments>
/*
as in 2a again replaced named instr by numbered.
seems to work but wrong output:
'Hello multiplestring string world!'
*/
instr 1 
S1 strget p4
S2 strget p5
S3 strget p6
S4 strget p7
S5 strget p8
printf_i "%s %s %s %s%s\n", 1, S1, S2, S3, S4, S5
endin
</CsInstruments>
<CsScore>
i 1 0 1 "Hello" "multiple" "string" "world" "!"
</CsScore>
</CsoundSynthesizer>
