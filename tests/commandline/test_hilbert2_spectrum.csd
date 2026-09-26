<CsTest>
description = "hilbert2 preserves the real signal through Nyquist for both output forms and every overlap"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 iFrequency = p4
 iFFTSize = p5
 iHop = p6
 iRoundedSize = p7
 aInput oscili .25, iFrequency, -1, .25
 aSine oscili .25, iFrequency
 aReference delay aInput, iRoundedSize/sr
 aQuadReference delay aSine, iRoundedSize/sr
 aReal, aImag hilbert2 aInput, iFFTSize, iHop
 kComplex:Complex[] hilbert2 aInput, iFFTSize, iHop
 kCycle init 0
 kCycle += 1
 ; Wait for all overlapping windows to contain the steady input.
 if kCycle > 24 then
  kSample = 0
  while kSample < ksmps do
   kReference vaget kSample, aReference
   kReal vaget kSample, aReal
   kImag vaget kSample, aImag
   kError = max(abs(kReal-kReference), abs(real(kComplex[kSample])-kReference))
   if !(kError < .000002) then
    printks "frequency %g, FFT %g, hop %g: real sample %g expected %g, array %g\n", 0, iFrequency, iFFTSize, iHop, kReal, kReference, real(kComplex[kSample])
    exitnowk(-1)
   endif
   ; Both forms must agree on the quadrature component as well.
   if !(abs(kImag-imag(kComplex[kSample])) < .000002) then
    printks "hilbert2 quadrature outputs disagree\n", 0
    exitnowk(-1)
   endif
   if (iFrequency == 0 || iFrequency == sr/2) && !(abs(kImag) < .000002) then
    printks "DC and Nyquist must have no steady quadrature component\n", 0
    exitnowk(-1)
   endif
   if iFrequency == 2048 then
    ; Away from the spectrum edges, cosine must become a unit-gain sine.
    kQuadReference vaget kSample, aQuadReference
    if !(abs(kImag-kQuadReference) < .000002) then
     printks "FFT %g, hop %g: quadrature %g expected %g\n", 0, iFFTSize, iHop, kImag, kQuadReference
     exitnowk(-1)
    endif
   endif
   kSample += 1
  od
 endif
 if kCycle == 64 then
  gkChecks += 1
 endif
endin

instr 99
 if i(gkChecks) != 19 then
  prints "hilbert2 spectrum checks did not finish\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Frequency, requested FFT size, requested hop, resulting FFT size.
; DC, middle, and the two highest positive-frequency bins, then Nyquist.
i 1 0 .125 0 128 32 128
i 1 0 .125 2048 128 32 128
i 1 0 .125 3968 128 32 128
i 1 0 .125 4032 128 32 128
i 1 0 .125 4096 128 32 128
; More overlap, half overlap, and no overlap must all preserve the real signal.
i 1 0 .125 4096 128 16 128
i 1 0 .125 0 128 64 128
i 1 0 .125 2048 128 64 128
i 1 0 .125 4096 128 64 128
i 1 0 .125 0 128 128 128
i 1 0 .125 2048 128 128 128
i 1 0 .125 4096 128 128 128
; A frequency between FFT bins also checks the stated sample delay.
i 1 0 .125 437 128 32 128
i 1 0 .125 437 128 64 128
i 1 0 .125 437 128 128 128
; Preserve rounding down to powers of two and clamping the hop to the FFT size.
i 1 0 .125 4096 150 35 128
i 1 0 .125 2048 128 256 128
; Small transforms must keep their Nyquist bin too.
i 1 0 .125 4096 8 2 8
i 1 0 .125 4096 2 1 2
i 99 .14 .002
e
</CsScore>
</CsoundSynthesizer>
