<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iOffset = int(p2*sr + .5) % ksmps
  iLegacy[] fillarray -.248472238, -.387805123, -.349853654, -.434099817
  kCount init 0
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart + 64 - kCount)
  aAmplitude init 0
  kN = 0
  while kN < ksmps do
    kAmplitude = 0
    if kN >= kStart && kN < kEnd then
      kAmplitude = .25 + (kCount + kN - kStart)/64
    endif
    vaset kAmplitude, kN, aAmplitude
    kN += 1
  od
  aReference pinkish 1, 0, p4, 1234
  if p5 == 0 then
    aActual pinkish aAmplitude, 0, p4, 1234
  else
    aAmplitude pinkish aAmplitude, 0, p4, 1234
    aActual = aAmplitude
  endif
  kN = 0
  while kN < ksmps do
    kActual vaget kN, aActual
    kReference vaget kN, aReference
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount + kN - kStart
      kExpected = kReference*(.25 + kSample/64)
      if p4 == 20 && kSample < 4 then
        if !(abs(kReference - iLegacy[kSample]) < .000001) then
          printks "FAIL pinkish changed the seeded sequence\n", 0
          exitnowk -1
        endif
      endif
    elseif kReference != 0 then
      printks "FAIL pinkish control amplitude left an inactive sample\n", 0
      exitnowk -1
    endif
    if !(abs(kActual - kExpected) < .000001) then
      printks "FAIL pinkish bands=%g offset=%g reuse=%g sample=%g: %g expected %g\n", \
          0, p4, iOffset, p5, kCount + kN - kStart, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd - kStart
  if kCount == 64 then
    gkChecks += 1
  endif
endin

instr 2
  ; Seeds with the same low 32 bits must produce the same sequence.
  aActual pinkish 1, 0, 20, p4
  aReference pinkish 1, 0, 20, p5
  kError max_k abs(aActual - aReference), 1, 1
  if !(kError < .000001) then
    printks "FAIL pinkish seed=%g differs from seed=%g by %g\n", 0, p4, p5, kError
    exitnowk -1
  endif
  kBlock init 0
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 3
  aInput = .5
  aReference pinkish aInput, p4, 20, 1234
  kBlock init 0
  kSkip init 0
  if kBlock == 2 then
    kSkip = p5
    reinit RESET
  endif
RESET:
  aActual pinkish aInput, p4, 20, 1234, i(kSkip)
  rireturn
  kError max_k abs(aActual - aReference), 1, 1
  if !(kError < .000001) then
    printks "FAIL pinkish method=%g skip=%g reset its state\n", 0, p4, p5
    exitnowk -1
  endif
  kBlock += 1
  if kBlock == 4 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 18 then
    prints "FAIL pinkish checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Aligned and partial blocks, with and without input reuse.
i 1 0 .0625 20 0
i 1 .1279296875 .0625 20 0
i 1 .2529296875 .0625 20 1
i 1 .3779296875 .0625 4 0
i 1 .5029296875 .0625 31 0
i 1 .6279296875 .0625 32 1
; Fractional, negative, and whole-cycle seed normalization.
i 2 .75 .0625 -.5 3221225472
i 2 .875 .0625 -1024 4294966272
i 2 1 .0625 -1024.5 4294966272
i 2 1.125 .0625 .5 1073741824
i 2 1.25 .0625 4294968320 1024
i 2 1.375 .0625 1234.5 1234
; All methods must preserve state for any nonzero skip value.
i 3 1.5 .0625 0 2
i 3 1.625 .0625 0 -1
i 3 1.75 .0625 1 2
i 3 1.875 .0625 1 -1
i 3 2 .0625 2 2
i 3 2.125 .0625 2 -1
i 99 2.25 .015625
</CsScore>
</CsoundSynthesizer>
