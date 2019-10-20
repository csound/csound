<CsoundSynthesizer>
<CsInstruments>
; By: Jonathan Murphy 2007

instr 1
    Smember strget p4

    ; Parse Smember
    istrlen    strlen   Smember
    idelimiter strindex Smember, ":"

    S1    strsub Smember, 0, idelimiter  ; "String1"
    S2    strsub Smember, idelimiter + 1, istrlen  ; "String2"

    printf "First string: %s\nSecond string: %s\n", 1, S1, S2

endin

</CsInstruments>
<CsScore>
i 1 0 1 "String1:String2"
</CsScore>
</CsoundSynthesizer>
