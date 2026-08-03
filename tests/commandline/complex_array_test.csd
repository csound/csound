<CsoundSynthesizer>
<CsOptions>
-n
</CsOptions>
<CsInstruments>

0dbfs = 1

opcode assert,0,kk
k1, k2 xin
if k1 != k2 then
 printks "assert error\n", 1
 exitnowk(-1)
endif
endop

opcode assert_close,0,kkk
kActual, kExpected, kTolerance xin
if abs(kActual - kExpected) > kTolerance then
 printks "assert_close error %.9f %.9f\n", 1, kActual, kExpected
 exitnowk(-1)
endif
endop

instr 1
 sig:Complex[] hilbert oscili(p4,p5)
 mod:Complex[] = oscili(0.5,100,-1,0.25), oscili(0.5,100,-1)
 ssb:Complex[] = mod * sig
   out real(ssb)
endin


instr 2
 ca:Complex[] init 2
 ca += [complex(1,1), complex(1,1)]
 assert(real(ca[0]), 1)
 assert(imag(ca[1]), 1)
 ca *= [complex(2,0), complex(2,0)]
 assert(real(ca[0]), 2)
 assert(imag(ca[1]), 2)
 ca /= [complex(2,0), complex(2,0)]
 assert(real(ca[0]), 1)
 assert(imag(ca[1]), 1)
 ca -= [complex(1,1), complex(1,1)]
 assert(real(ca[0]), 0)
 assert(imag(ca[1]), 0)
endin

opcode reorder(s:Complex[]):void
  N:k = lenarray(s)
  i:k, j:k, m:k init 0, 0, 0
  while i < N do
   if j > i then
     tmp:Complex = s[i]
     s[i] = s[j]
     s[j] = tmp
   endif
   m = N/2
   while m >= 2 && j >= m do
     j -= m
     m *= 0.5
   od
   j += m
   i += 1
  od
endop

opcode FFT(s:Complex[],fwd:b):void
 N:k = lenarray(s)
 n:k = 1
 pi:i = fwd == true ? -$M_PI : $M_PI
 reorder(s)
 while n < N do
  o:k = pi/n
  wp:Complex = cos(o), sin(o)
  w:Complex = 1,0
  m:k = 0
  n2:k = n*2 
  while m < n do
   k = m
   while k < N do 
     i:k = k  + n
     even:Complex = s[k]
     odd:Complex = w*s[i]
     s[k] = even + odd
     s[i] = even - odd
     k += n2
   od
   w *= wp
   m += 1
  od 
  n *= 2
 od
 if fwd then
  s = s/N
 endif
endop

instr 3
s:Complex[] init 16
n:k = 0
while n < 16 do
  s[n] = cos(2*$M_PI * n/16), 0
  n+=1
od
FFT(s,true)
if abs(s[1]) != 0.5 then
  printks "FFT fail\n", 1
  exitnowk(-1)
else
 n = 0;
 while n < 16 do
  printk2 abs(s[n])
  n+=1
 od
endif
turnoff
endin

instr 4
kRectInput:Complex[] = [complex(0, 2, 0)]
kPolarInput:Complex[] = [complex(2, $M_PI/2, 1)]
kRectLeft:Complex[] = kRectInput + 1
kRectRight:Complex[] = 1 + kRectInput
kPolarLeft:Complex[] = kPolarInput + 1
kPolarRight:Complex[] = 1 + kPolarInput
kTolerance = 0.00001

assert_close(real(kRectLeft[0]), 1, kTolerance)
assert_close(imag(kRectLeft[0]), 2, kTolerance)
assert_close(abs(kRectLeft[0]), sqrt(5), kTolerance)
assert_close(arg(kRectLeft[0]), taninv2(2, 1), kTolerance)

assert_close(real(kRectRight[0]), 1, kTolerance)
assert_close(imag(kRectRight[0]), 2, kTolerance)
assert_close(abs(kRectRight[0]), sqrt(5), kTolerance)
assert_close(arg(kRectRight[0]), taninv2(2, 1), kTolerance)

assert_close(real(kPolarLeft[0]), 1, kTolerance)
assert_close(imag(kPolarLeft[0]), 2, kTolerance)
assert_close(abs(kPolarLeft[0]), sqrt(5), kTolerance)
assert_close(arg(kPolarLeft[0]), taninv2(2, 1), kTolerance)

assert_close(real(kPolarRight[0]), 1, kTolerance)
assert_close(imag(kPolarRight[0]), 2, kTolerance)
assert_close(abs(kPolarRight[0]), sqrt(5), kTolerance)
assert_close(arg(kPolarRight[0]), taninv2(2, 1), kTolerance)
turnoff
endin

instr 5
kTolerance = 0.000001
kMagnitude[] = [2, 3, 0]
kPhase[] = [0, $M_PI/2, 1.25]
kValues:Complex[] = polar(kMagnitude, kPhase)
assert_close(real(kValues[0]), 2, kTolerance)
assert_close(imag(kValues[0]), 0, kTolerance)
assert_close(real(kValues[1]), 0, kTolerance)
assert_close(imag(kValues[1]), 3, kTolerance)
assert_close(abs(kValues[2]), 0, kTolerance)
assert_close(arg(kValues[2]), 1.25, kTolerance)

kLongMagnitude[] = [4, 5, 6]
kShortPhase[] = [0.25, 0.5]
kLongFirst:Complex[] = polar(kLongMagnitude, kShortPhase)
assert(lenarray(kLongFirst), 3)
assert_close(abs(kLongFirst[0]), 4, kTolerance)
assert_close(arg(kLongFirst[1]), 0.5, kTolerance)
assert_close(real(kLongFirst[2]), 0, kTolerance)
assert_close(imag(kLongFirst[2]), 0, kTolerance)

kShortMagnitude[] = [7, 8]
kLongPhase[] = [0, 0.75, 1]
kShortFirst:Complex[] = polar(kShortMagnitude, kLongPhase)
assert(lenarray(kShortFirst), 2)
assert_close(abs(kShortFirst[1]), 8, kTolerance)
assert_close(arg(kShortFirst[1]), 0.75, kTolerance)
turnoff
endin

</CsInstruments>
<CsScore>
i4 0 1
i5 0 1
i3 0 1
i2 0 1
i1 0 1 0.5 400
</CsScore>
</CsoundSynthesizer>
