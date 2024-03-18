<CsoundSynthesizer>
<CsOptions>
-odac
</CsOptions>
<CsInstruments>

instr 1
Sdest[] init 2
Stype[] init 2
kdata[][] init 2, 2

Sdest[0] = "/test/floats"
Sdest[1] = "/test/ints"
Stype[0] = "ff"
Stype[1] = "ii"

kdata fillarray 1,2,3,4

OSCbundle  1, "localhost", 7000, Sdest, Stype, kdata
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>