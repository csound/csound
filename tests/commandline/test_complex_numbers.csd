<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode assert,0,kk
k1, k2 xin
if k1 != k2 then
 printks "assert error\n", 1
 exitnowk(-1)
endif
endop

instr 1
 Ca:Complex[] init 10
 Ca2:Complex[] = [complex(1,1), complex(2,2)]
 ca:Complex = 0,1
 cc:Complex = Ca2[0]*Ca2[1]
 assert(real(ca),0)
 assert(imag(ca),1)
 cb:Complex = polar(ca)
 assert(arg(cb), $M_PI/2)
 Ca[0] = ca * ca
 Cb:Complex[] = Ca + Ca
 cc:Complex = Ca[0]
 assert(abs(cc), 1)
endin

</CsInstruments>
<CsScore>
i1 0 1
</CsScore>
</CsoundSynthesizer>
