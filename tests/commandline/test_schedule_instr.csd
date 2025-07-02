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
MyInst:Instr schedule 1, 0, 0, p4
print p4
endin

MyInst:Instr schedule One, 0, 1, 2
MyInst2:Instr schedule "Two", 0, 1 ,2

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>


