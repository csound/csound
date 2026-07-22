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

opcode assert_close,0,kkk
kactual, kexpected, ktolerance xin
if abs(kactual - kexpected) > ktolerance then
 printks "assert_close error %f %f\n", 1, kactual, kexpected
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

instr 5
kReal[] = [10, 4]
kRect:Complex[] = [complex(3, 4), complex(-1, 2)]
kPolar:Complex[] = [complex(5, taninv2(4, 3), 1), complex(2, -$M_PI/2, 1)]
kepsilon = 0.00001

kResult:Complex[] = kRect / kReal
assert_close(real(kResult[0]), 0.3, kepsilon)
assert_close(imag(kResult[0]), 0.4, kepsilon)
assert_close(real(kResult[1]), -0.25, kepsilon)
assert_close(imag(kResult[1]), 0.5, kepsilon)

kResult = kReal / kRect
assert_close(real(kResult[0]), 1.2, kepsilon)
assert_close(imag(kResult[0]), -1.6, kepsilon)
assert_close(real(kResult[1]), -0.8, kepsilon)
assert_close(imag(kResult[1]), -1.6, kepsilon)

kResult = kPolar / kReal
assert_close(real(kResult[0]), 0.3, kepsilon)
assert_close(imag(kResult[0]), 0.4, kepsilon)
assert_close(real(kResult[1]), 0, kepsilon)
assert_close(imag(kResult[1]), -0.5, kepsilon)

kResult = kReal / kPolar
assert_close(real(kResult[0]), 1.2, kepsilon)
assert_close(imag(kResult[0]), -1.6, kepsilon)
assert_close(real(kResult[1]), 0, kepsilon)
assert_close(imag(kResult[1]), 2, kepsilon)
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
i3 0 1
i4 0 1
i5 0 1
</CsScore>
</CsoundSynthesizer>
