<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>

<CsInstruments>
instr 1
    Swd pwd
        printf_i "Working directory is '%s'\n", 1, Swd
        prints "Reading myself =):\n"
read:
Sline, iLineNum readfi "readfi.csd"
        printf_i  "Line %d: %s", iLineNum, iLineNum, Sline
        if iLineNum != -1 igoto read
endin
</CsInstruments>

<CsScore>
i1 0 0.1
e
</CsScore>

</CsoundSynthesizer>
