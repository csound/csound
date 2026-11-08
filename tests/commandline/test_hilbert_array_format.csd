<CsTest>
description = "hilbert array format and length after output reuse"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
instr 1
  aInput oscili .1,437
  aReal,aImag hilbert aInput
  kArray:Complex[] hilbert aInput
  kCycle init 0
  kPeak init 0
  kLength lenarray kArray
  if kLength != ksmps then
    printks "hilbert output length %g, expected %g\n",0,kLength,ksmps
    exitnowk(-1)
  endif
  kIndex = 0
  while kIndex < ksmps do
    kReal vaget kIndex,aReal
    kImag vaget kIndex,aImag
    kError = abs(kReal-real(kArray[kIndex])) + abs(kImag-imag(kArray[kIndex]))
    if !(kError < .000001) then
      printks "hilbert complex output mismatch at cycle %g, sample %g: %g\n",0,kCycle,kIndex,kError
      exitnowk(-1)
    endif
    kPeak = max(kPeak,abs(kReal),abs(kImag))
    ; Leave every element in polar form for the next block to overwrite.
    kArray[kIndex] = polar(kArray[kIndex])
    kIndex += 1
  od
  ; Both retain capacity; the producer must restore one element per sample.
  if kCycle % 3 == 1 then
    trim kArray,3
  elseif kCycle % 3 == 2 then
    trim kArray,0
  endif
  kCycle += 1
  if kCycle == ceil((p3*sr+(round(p2*sr)%ksmps))/ksmps) then
    if !(kPeak > .01) then
      printks "hilbert reference stayed silent\n",0
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin
instr 2
  aInput oscili .1,437
  aReal,aImag hilbert2 aInput,128,32
  kArray:Complex[] hilbert2 aInput,128,32
  kCycle init 0
  kPeak init 0
  kLength lenarray kArray
  if kLength != ksmps then
    printks "hilbert2 output length %g, expected %g\n",0,kLength,ksmps
    exitnowk(-1)
  endif
  kIndex = 0
  while kIndex < ksmps do
    kReal vaget kIndex,aReal
    kImag vaget kIndex,aImag
    kError = abs(kReal-real(kArray[kIndex])) + abs(kImag-imag(kArray[kIndex]))
    if !(kError < .000001) then
      printks "hilbert2 complex output mismatch at cycle %g, sample %g: %g\n",0,kCycle,kIndex,kError
      exitnowk(-1)
    endif
    kPeak = max(kPeak,abs(kReal),abs(kImag))
    ; Leave every element in polar form for the next block to overwrite.
    kArray[kIndex] = polar(kArray[kIndex])
    kIndex += 1
  od
  ; Both retain capacity; the producer must restore one element per sample.
  if kCycle % 3 == 1 then
    trim kArray,3
  elseif kCycle % 3 == 2 then
    trim kArray,0
  endif
  kCycle += 1
  if kCycle == ceil((p3*sr+(round(p2*sr)%ksmps))/ksmps) then
    if !(kPeak > .01) then
      printks "hilbert2 reference stayed silent\n",0
      exitnowk(-1)
    endif
    gkChecks += 1
  endif
endin
instr 99
  if i(gkChecks) != 4 then
    prints "hilbert complex output checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .125
i 2 .25 .125
i 1 .5003662109375 .125
i 2 .7503662109375 .125
i 99 1 .01
e
</CsScore>
</CsoundSynthesizer>
