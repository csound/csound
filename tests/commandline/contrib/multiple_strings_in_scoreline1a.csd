<CsoundSynthesizer>
<CsOptions>
-+max_str_len=1000 ;this does not help
</CsOptions>
<CsInstruments>
/*
testing long strings
looks like this is the reason for the bug
*/
instr testimonium
S1 strget p4
S2 strget p5
S3 strget p6
printf_i "%s %s%s\n", 1, S1, S2, S3
endin
</CsInstruments>
<CsScore>
i "testimonium" 0 1 "HelloBelloMello" "worldlroworldlrow" "!?=)((/()=!!"
</CsScore>
</CsoundSynthesizer>
