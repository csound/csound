<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode assert,0,ii
i1, i2 xin
if i1 != i2 then
 prints "assert error\n"
 exitnow(-1)
endif
endop

instr 1
 Ca:Complex[] init 10
 ca:Complex = 0,1
 assert(real(ca),0)
 assert(imag(ca),1)
 cb:Complex = polar(ca)
 assert(arg(cb), $M_PI/2)
 Ca[0] = ca * ca
 cc:Complex = Ca[0]
 assert(abs(cc), 1)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
