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
k1 init 0
if k1 > 0 kgoto end
 printk 0.1, k1
 event "i", 1, 0, 0, p4
end:
k1 = p4
endin

event_i "i", One, 0, 1, 2
event_i "i","Two", 0, 1 ,2

</CsInstruments>
<CsScore>
f 0 1
</CsScore>
</CsoundSynthesizer>


