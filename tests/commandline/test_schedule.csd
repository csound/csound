<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

nchnls = 1
0dbfs = 1

instr One
if p4 != 2 then
 exitnow(-1)
endif
print p4
endin

instr Two
S1 = p4
i1 strcmp S1, "test"
if i1 != 0 then
 exitnow(-1)
endif
print i1
endin

instr Three
k1 init 0
if k1 == 0 then
schedulek 1, 0, 1, 2
schedulek One, 0, 1, 2
schedulek "One", 0, 1, 2
endif
print p4
k1 = p4
endin

iArgs[] fillarray 1,0,1,2
schedule iArgs
schedule 1, 0, 1, 2
schedule One, 0, 1, 2
schedule "One", 0, 1, 2

schedule 2, 0, 1, "test"
schedule Two, 0, 1, "test"
schedule "Two", 0, 1, "test"

schedule "Three", 0, 1, 2

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>


