<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

instr 1
Ca:Complex[] init 10
ca:Complex = 2,1
print real(ca)
cb:Complex = polar(ca)
print arg(cb)
Ca[0] = ca * ca
cc:Complex = Ca[0]
print abs(cc)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
