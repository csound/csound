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

kdifference = abs(kactual - kexpected)
if qnan(kdifference) != 0 || kdifference > ktolerance then
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
kARect:Complex = complex(1, 2)
kAPolar:Complex = complex(sqrt(5), taninv2(2, 1), 1)
kBRect:Complex = complex(3, -4)
kBPolar:Complex = complex(5, taninv2(-4, 3), 1)
kLeft:Complex[] = [kARect, kARect, kAPolar, kAPolar]
kRight:Complex[] = [kBRect, kBPolar, kBRect, kBPolar]
kepsilon = 0.00001

kAdd:Complex[] = kLeft + kRight
kSub:Complex[] = kLeft - kRight
kMul:Complex[] = kLeft * kRight
kDiv:Complex[] = kLeft / kRight
kIndex = 0
while kIndex < 4 do
 assert_close(real(kAdd[kIndex]), 4, kepsilon)
 assert_close(imag(kAdd[kIndex]), -2, kepsilon)
 assert_close(real(kSub[kIndex]), -2, kepsilon)
 assert_close(imag(kSub[kIndex]), 6, kepsilon)
 assert_close(real(kMul[kIndex]), 11, kepsilon)
 assert_close(imag(kMul[kIndex]), 2, kepsilon)
 assert_close(real(kDiv[kIndex]), -0.2, kepsilon)
 assert_close(imag(kDiv[kIndex]), 0.4, kepsilon)
 kIndex += 1
od

kAdd = kRight
kSub = kRight
kMul = kRight
kDiv = kRight
kAdd += kLeft
kSub -= kLeft
kMul *= kLeft
kDiv /= kLeft
kIndex = 0
while kIndex < 4 do
 assert_close(real(kAdd[kIndex]), 4, kepsilon)
 assert_close(imag(kAdd[kIndex]), -2, kepsilon)
 assert_close(real(kSub[kIndex]), 2, kepsilon)
 assert_close(imag(kSub[kIndex]), -6, kepsilon)
 assert_close(real(kMul[kIndex]), 11, kepsilon)
 assert_close(imag(kMul[kIndex]), 2, kepsilon)
 assert_close(real(kDiv[kIndex]), -1, kepsilon)
 assert_close(imag(kDiv[kIndex]), -2, kepsilon)
 kIndex += 1
od
endin

instr 6
kARect:Complex = complex(1, 2)
kAPolar:Complex = complex(sqrt(5), taninv2(2, 1), 1)
kBPolar:Complex = complex(5, taninv2(-4, 3), 1)
kValues:Complex[] = [kARect, kAPolar]
kepsilon = 0.00001

kAddLeft:Complex[] = kValues + kBPolar
kAddRight:Complex[] = kBPolar + kValues
kSubLeft:Complex[] = kValues - kBPolar
kSubRight:Complex[] = kBPolar - kValues
kMulLeft:Complex[] = kValues * kBPolar
kMulRight:Complex[] = kBPolar * kValues
kDiv:Complex[] = kValues / kBPolar
kIndex = 0
while kIndex < 2 do
 assert_close(real(kAddLeft[kIndex]), 4, kepsilon)
 assert_close(imag(kAddLeft[kIndex]), -2, kepsilon)
 assert_close(real(kAddRight[kIndex]), 4, kepsilon)
 assert_close(imag(kAddRight[kIndex]), -2, kepsilon)
 assert_close(real(kSubLeft[kIndex]), -2, kepsilon)
 assert_close(imag(kSubLeft[kIndex]), 6, kepsilon)
 assert_close(real(kSubRight[kIndex]), 2, kepsilon)
 assert_close(imag(kSubRight[kIndex]), -6, kepsilon)
 assert_close(real(kMulLeft[kIndex]), 11, kepsilon)
 assert_close(imag(kMulLeft[kIndex]), 2, kepsilon)
 assert_close(real(kMulRight[kIndex]), 11, kepsilon)
 assert_close(imag(kMulRight[kIndex]), 2, kepsilon)
 assert_close(real(kDiv[kIndex]), -0.2, kepsilon)
 assert_close(imag(kDiv[kIndex]), 0.4, kepsilon)
 kIndex += 1
od
endin

instr 51
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

instr 62
kReal[] = [10]
kRect:Complex[] = [complex(3, 4)]
kPolar:Complex[] = [complex(5, taninv2(4, 3), 1)]
kepsilon = 0.00001

kResult:Complex[] = kRect - kReal
assert_close(real(kResult[0]), -7, kepsilon)
assert_close(imag(kResult[0]), 4, kepsilon)

kResult = kReal - kRect
assert_close(real(kResult[0]), 7, kepsilon)
assert_close(imag(kResult[0]), -4, kepsilon)

kResult = kPolar - kReal
assert_close(real(kResult[0]), -7, kepsilon)
assert_close(imag(kResult[0]), 4, kepsilon)

kResult = kReal - kPolar
assert_close(real(kResult[0]), 7, kepsilon)
assert_close(imag(kResult[0]), -4, kepsilon)
endin

instr 71
kEpsilon = 0.00001
kRect:Complex = complex(3, 4)
kPolar:Complex = complex(5, taninv2(4, 3), 1)

kResult:Complex = kRect - 10
assert_close(real(kResult), -7, kEpsilon)
assert_close(imag(kResult), 4, kEpsilon)

kResult = 10 - kRect
assert_close(real(kResult), 7, kEpsilon)
assert_close(imag(kResult), -4, kEpsilon)

kResult = kPolar - 10
assert_close(real(kResult), -7, kEpsilon)
assert_close(imag(kResult), 4, kEpsilon)

kResult = 10 - kPolar
assert_close(real(kResult), 7, kEpsilon)
assert_close(imag(kResult), -4, kEpsilon)
endin

instr 61
Csource:Complex[] = [complex(3,4), polar(complex(3,4))]
Cpolar:Complex[] = polar(Csource)
kepsilon = 0.00001

assert_close(abs(Cpolar[0]), abs(Csource[0]), kepsilon)
assert_close(arg(Cpolar[0]), arg(Csource[0]), kepsilon)
assert_close(real(Cpolar[0]), real(Csource[0]), kepsilon)
assert_close(imag(Cpolar[0]), imag(Csource[0]), kepsilon)

assert_close(abs(Cpolar[1]), abs(Csource[1]), kepsilon)
assert_close(arg(Cpolar[1]), arg(Csource[1]), kepsilon)
assert_close(real(Cpolar[1]), real(Csource[1]), kepsilon)
assert_close(imag(Cpolar[1]), imag(Csource[1]), kepsilon)
endin

instr 7
Csource:Complex[] = [complex(5, taninv2(4, 3), 1), complex(-2, 7)]
Crect:Complex[] = complex(Csource)
kepsilon = 0.00001

assert_close(real(Crect[0]), 3, kepsilon)
assert_close(imag(Crect[0]), 4, kepsilon)
assert_close(abs(Crect[0]), 5, kepsilon)
assert_close(arg(Crect[0]), taninv2(4, 3), kepsilon)

assert_close(real(Crect[1]), -2, kepsilon)
assert_close(imag(Crect[1]), 7, kepsilon)
assert_close(abs(Crect[1]), sqrt(53), kepsilon)
assert_close(arg(Crect[1]), taninv2(7, -2), kepsilon)
endin

</CsInstruments>
<CsScore>
i1 0 1
i2 0 1
i3 0 1
i4 0 1
i5 0 1
i6 0 1
i51 0 1
i61 0 1
i62 0 1
i71 0 1
i7 0 1
</CsScore>
</CsoundSynthesizer>
