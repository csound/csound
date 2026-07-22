<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

opcode assert,0,kk
k1, k2 xin
if k1 != k2 then
 printks "assert error %f %f\n", 1, k1, k2
 exitnowk(-1)
endif
endop

opcode assert_close,0,kk
kActual, kExpected xin
if abs(kActual - kExpected) > 0.000001 then
 printks "assert close error %.9f %.9f\n", 1, kActual, kExpected
 exitnowk(-1)
endif
endop

test@global:Complex[] init 2
test[0] = 1,2
test[1] = 3,4


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

 kAddOnce:Complex = complex(1, 2)
 kAddOnce += complex(3, 4)
 assert_close(real(kAddOnce), 4)
 assert_close(imag(kAddOnce), 6)

 kSubOnce:Complex = complex(5, 7)
 kSubOnce -= complex(2, 3)
 assert_close(real(kSubOnce), 3)
 assert_close(imag(kSubOnce), 4)

 kMulOnce:Complex = complex(1, 2)
 kMulOnce *= complex(3, 4)
 assert_close(real(kMulOnce), -5)
 assert_close(imag(kMulOnce), 10)

 kDivRR:Complex = complex(4*cos(0.75), 4*sin(0.75), 0)
 kDivRR /= complex(2*cos(0.25), 2*sin(0.25), 0)
 assert_close(abs(kDivRR), 2)
 assert_close(arg(kDivRR), 0.5)
 assert_close(real(kDivRR), 2*cos(0.5))
 assert_close(imag(kDivRR), 2*sin(0.5))

 kDivRP:Complex = complex(4*cos(0.75), 4*sin(0.75), 0)
 kDivRP /= complex(2, 0.25, 1)
 assert_close(abs(kDivRP), 2)
 assert_close(arg(kDivRP), 0.5)
 assert_close(real(kDivRP), 2*cos(0.5))
 assert_close(imag(kDivRP), 2*sin(0.5))

 kDivPR:Complex = complex(4, 0.75, 1)
 kDivPR /= complex(2*cos(0.25), 2*sin(0.25), 0)
 assert_close(abs(kDivPR), 2)
 assert_close(arg(kDivPR), 0.5)
 assert_close(real(kDivPR), 2*cos(0.5))
 assert_close(imag(kDivPR), 2*sin(0.5))

 kDivPP:Complex = complex(4, 0.75, 1)
 kDivPP /= complex(2, 0.25, 1)
 assert_close(abs(kDivPP), 2)
 assert_close(arg(kDivPP), 0.5)
 assert_close(real(kDivPP), 2*cos(0.5))
 assert_close(imag(kDivPP), 2*sin(0.5))
 turnoff
endin

instr 3
Ca:Complex[] = [ complex(1,0), complex(0,1) ]
Ra:k[] = [1, 2]
Ca += Ra
assert(real(Ca[0]), real(Ca[1]))
Ca *= Ra
assert(real(Ca[0])*2, real(Ca[1]))
Ca /= Ra
assert(real(Ca[0]), real(Ca[1]))
Ca -= Ra
assert(real(Ca[0])-1, real(Ca[1]))
assert(imag(Ca[0]), 0)
assert(imag(Ca[1]), 1)
endin

instr 4
Ca1:Complex[] = [ complex(1,0), complex(0,1), complex(0,0) ]
Ca:Complex[] = [ complex(1,0), complex(0,1) ]
Ra:k[] = [1, 2, 3]
Ca += Ra
Ca = 2*Ca/Ca1
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
i3 0 1
i4 0 1
</CsScore>
</CsoundSynthesizer>
