<CsTest>
description = "Scalar complex sums and differences preserve values across representations and reused operands"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
gkCycles init 0

opcode Check, 0, kkkkS
  kReal, kImag, kExpectedReal, kExpectedImag, SCase xin
  ; The negated comparison also rejects NaN results.
  if !(abs(kReal - kExpectedReal) < 0.00001 && abs(kImag - kExpectedImag) < 0.00001) then
    printks "%s: expected (%g, %g), got (%g, %g)\n", 0, SCase, kExpectedReal, kExpectedImag, kReal, kImag
    exitnowk -1
  endif
endop

instr 1
  ; p4 and p5 choose rectangular (0) or polar (1) storage.
  ; All four cases represent the same operands: (2 + 3i) and (1 + i).
  kLeft:Complex = complex(p4 == 0 ? 2 : sqrt(13), p4 == 0 ? 3 : taninv2(3, 2), p4)
  kRight:Complex = complex(p5 == 0 ? 1 : sqrt(2), p5 == 0 ? 1 : $M_PI/4, p5)
  SCase sprintf "left polar=%d, right polar=%d", p4, p5

  kSum:Complex = kLeft + kRight
  kDifference:Complex = kLeft - kRight
  ; A fresh result can work at init but fail when reused at performance time.
  iSumReal = real(kSum)
  iSumImag = imag(kSum)
  iDifferenceReal = real(kDifference)
  iDifferenceImag = imag(kDifference)
  if !(abs(iSumReal - 3) < 0.00001 && abs(iSumImag - 4) < 0.00001) then
    prints "Incorrect init-time sum\n"
    exitnow -1
  endif
  if !(abs(iDifferenceReal - 1) < 0.00001 && abs(iDifferenceImag - 2) < 0.00001) then
    prints "Incorrect init-time difference\n"
    exitnow -1
  endif
  Check real(kSum), imag(kSum), 3, 4, strcat(SCase, " +")
  Check real(kDifference), imag(kDifference), 1, 2, strcat(SCase, " -")

  ; Compound assignment must read both components before changing either.
  kAddInPlace:Complex = kLeft
  kAddInPlace += kRight
  Check real(kAddInPlace), imag(kAddInPlace), 3, 4, strcat(SCase, " +=")
  kSubtractInPlace:Complex = kLeft
  kSubtractInPlace -= kRight
  Check real(kSubtractInPlace), imag(kSubtractInPlace), 1, 2, strcat(SCase, " -=")

  ; Ordinary assignment may reuse either operand as the result.
  kReuseLeft:Complex = kLeft
  kReuseLeft = kReuseLeft + kRight
  Check real(kReuseLeft), imag(kReuseLeft), 3, 4, strcat(SCase, " left = left + right")
  kReuseRight:Complex = kRight
  kReuseRight = kLeft + kReuseRight
  Check real(kReuseRight), imag(kReuseRight), 3, 4, strcat(SCase, " right = left + right")
  kReuseLeft = kLeft
  kReuseLeft = kReuseLeft - kRight
  Check real(kReuseLeft), imag(kReuseLeft), 1, 2, strcat(SCase, " left = left - right")
  kReuseRight = kRight
  kReuseRight = kLeft - kReuseRight
  Check real(kReuseRight), imag(kReuseRight), 1, 2, strcat(SCase, " right = left - right")

  ; The source and destination can also be the very same complex value.
  kSelf:Complex = kLeft
  kSelf += kSelf
  Check real(kSelf), imag(kSelf), 4, 6, strcat(SCase, " self += self")
  kSelf = kLeft
  kSelf -= kSelf
  Check real(kSelf), imag(kSelf), 0, 0, strcat(SCase, " self -= self")

  gkCycles += 1
  kCycle timeinstk
  if kCycle == 3 then
    turnoff
  endif
endin

instr 99
  ; Four operand combinations must each run for three control cycles.
  if i(gkCycles) != 12 then
    prints "Complex arithmetic checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 0
i 1 0 .01 0 1
i 1 0 .01 1 0
i 1 0 .01 1 1
i 99 .02 .001
e
</CsScore>
</CsoundSynthesizer>
