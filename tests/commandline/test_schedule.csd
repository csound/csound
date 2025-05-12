<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

nchnls = 1
0dbfs = 1

instr One, 1
if p4 != 2 then
 exitnow(-1)
endif
print p4
endin

instr Two, 2
S1 = p4
i1 strcmp S1, "test"
if i1 != 0 then
 exitnow(-1)
endif
print i1
endin

iArgs[] fillarray 1,0,1,2
schedule iArgs
schedule 1, 0, 1, 2
schedule One, 0, 1, 2
schedule "One", 0, 1, 2

schedule 2, 0, 1, "test"
schedule Two, 0, 1, "test"
schedule "Two", 0, 1, "test"

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>


