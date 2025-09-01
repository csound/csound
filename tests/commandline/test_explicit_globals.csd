<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

myvar@global:i init 0
myarr@global:i[] fillarray 1,2,3

instr 1
print myvar
print myarr[0]
myarr[1] = 0
endin

instr 2
var:i = 1
mtest@global:i = 2*var
endin

instr 3
if mtest != 2 then
 exitnow(-1)
endif
print mtest
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
i3 0 1
</CsScore>
</CsoundSynthesizer>
