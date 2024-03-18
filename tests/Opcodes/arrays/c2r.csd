<CsoundSynthesizer>
<CsOptions>
-d -o dac
</CsOptions>
<CsInstruments>
ksmps = 64

instr 1
ifftsize = 1024
kcnt init 0
kIn[] init  ifftsize
kOut[] init ifftsize

a1 oscili 0dbfs/2, 440

if kcnt >= ifftsize then
 kCmplx[] r2c kIn
 kSpec[] fft kCmplx
 kCmplx fftinv kSpec
 kOut c2r kCmplx
 kcnt = 0 
endif

kIn[] shiftin a1
a2 shiftout kOut
kcnt += ksmps
   out a2
endin
</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
