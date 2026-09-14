<CsoundSynthesizer>
<CsInstruments>
/* Csound-test
{
  "description": "contrib/multiple_strings_in_scoreline2a.csd",
  "expect": {
    "exit": 0
  }
}
*/
/*
now replacing instr test with instr 1.
works again
*/
instr 1
S1 strget p4
S2 strget p5
S3 strget p6
S4 strget p7
printf_i "%s %s %s%s\n", 1, S1, S2, S3, S4
endin
</CsInstruments>
<CsScore>
i 1 0 1 "Hello" "string" "world" "!"
</CsScore>
</CsoundSynthesizer>
