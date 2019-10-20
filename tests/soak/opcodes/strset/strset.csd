<CsoundSynthesizer>
<CsInstruments>

sr = 44100
ksmps = 1
nchnls = 1

;Example by Andres Cabrera 2008

; \\n is used to denote "new line" 
strset 1, "String 1\\n"
strset 2, "String 2\\n"

instr 1
Str strget p4
prints Str
endin


</CsInstruments>
<CsScore>
;        p4 is used to select string
i 1 0 1  1
i 1 3 1  2
</CsScore>
</CsoundSynthesizer>
