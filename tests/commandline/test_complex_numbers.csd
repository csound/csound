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
 ca:Complex = 0,1
 cc:Complex = complex(1,1)*complex(2,2)
 assert(real(ca),0)
 assert(imag(ca),1)
 cb:Complex = polar(ca)
 assert(arg(cb), $M_PI/2)
 Ca[0] = ca * ca
 Cb:Complex[] = Ca + Ca
 cc:Complex = Ca[0]
 assert(abs(cc), 1)
endin

instr 2
 ca:Complex = 0,1
 ca += complex(1,0)
 assert(real(ca), 1)
 assert(imag(ca), 1)
 ca *= complex(2,0)
 assert(real(ca), 2)
 assert(imag(ca), 2)
 ca /= complex(2,0)
 assert(real(ca), 1)
 assert(imag(ca), 1)
 ca -= complex(1,1)
 assert(real(ca), 0)
 assert(imag(ca), 0)
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
</CsScore>
</CsoundSynthesizer>
