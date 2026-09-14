<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 aInput oscili .1, 437
 aReal, aImag hilbert2 aInput, 128, 32
 aCopy = aInput
 aCopy, aCopyImag hilbert2 aCopy, 128, 32
 aCopy2 = aInput
 aCopyReal, aCopy2 hilbert2 aCopy2, 128, 32
 kArray:Complex[] hilbert2 aInput, 128, 32
 kCycle init 0
 kPeak init 0
 kN = 0
 while kN < ksmps do
  kReal vaget kN, aReal
  kImag vaget kN, aImag
  kCopy vaget kN, aCopy
  kCopyImag vaget kN, aCopyImag
  kCopyReal vaget kN, aCopyReal
  kCopy2 vaget kN, aCopy2
  kError = abs(kReal-kCopy) + abs(kImag-kCopyImag) + \
           abs(kReal-kCopyReal) + abs(kImag-kCopy2) + \
           abs(kReal-real(kArray[kN])) + abs(kImag-imag(kArray[kN]))
  if !(kError < 1e-6) then
   printks "hilbert2 input reuse/array mismatch: cycle=%g sample=%g error=%g\n", 0, kCycle, kN, kError
   exitnowk(-1)
  endif
  kPeak = max(kPeak, abs(kReal), abs(kImag))
  kN += 1
 od
 kCycle += 1
 if kCycle == 50 then
  if !(kPeak > .05) then
   printks "hilbert2 reference stayed silent\n", 0
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr 2
 aInput oscili .1, 437
 kSize init 128
 kCycle init 0
 kCycle += 1
 if kCycle == 10 then
  kSize = 64
  reinit TRANSFORMS
 endif
TRANSFORMS:
 iSize = i(kSize)
 aReal, aImag hilbert2 aInput, iSize, 16
 kArray:Complex[] hilbert2 aInput, iSize, 16
 ; Reinitializing the fixed-size reference resets its state without
 ; changing its window.  Both transforms must then agree sample for sample.
 aRefReal, aRefImag hilbert2 aInput, 64, 16
 rireturn
 if kCycle >= 10 then
  kN = 0
  while kN < ksmps do
   kReal vaget kN, aReal
   kImag vaget kN, aImag
   kRefReal vaget kN, aRefReal
   kRefImag vaget kN, aRefImag
   kError = abs(kReal-kRefReal) + abs(kImag-kRefImag) + \
            abs(real(kArray[kN])-kRefReal) + abs(imag(kArray[kN])-kRefImag)
   if !(kError < 1e-6) then
    printks "hilbert2 resized window mismatch: cycle=%g sample=%g error=%g\n", 0, kCycle, kN, kError
    exitnowk(-1)
   endif
   kN += 1
  od
 endif
 if kCycle == 50 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 3 then
  prints "hilbert2 checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .103375
i 1 .120625 .10275
i 2 .24 .104
i 99 .36 .01
e
</CsScore>
</CsoundSynthesizer>
