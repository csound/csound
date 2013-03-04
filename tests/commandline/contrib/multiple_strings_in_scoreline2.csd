<CsoundSynthesizer>
<CsInstruments>
/*
only one string more than in the previous example.
now the output is:
Parsing successful!
instr test uses instrument number 1
Elapsed time at end of orchestra compile: real: 0.002s, CPU: 0.000s
sorting score ...
WARNING: instr test not found, assuming insno = -1
*/
instr test
S1 strget p4
S2 strget p5
S3 strget p6
S4 strget p7
printf_i "%s %s %s %s%s\n", 1, S1, S2, S3, S4
endin
</CsInstruments>
<CsScore>
i "test" 0 1 "Hello" "string" "world" "!"
</CsScore>
</CsoundSynthesizer>
