<CsTest>
description = "contrib/multiple_strings_in_scoreline1.csd"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsInstruments>
instr test ;fine
S1 strget p4
S2 strget p5
S3 strget p6
printf_i "%s %s%s\n", 1, S1, S2, S3
endin
</CsInstruments>
<CsScore>
i "test" 0 1 "Hello" "world" "!"
</CsScore>
</CsoundSynthesizer>
