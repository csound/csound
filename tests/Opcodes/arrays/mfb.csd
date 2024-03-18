<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
ksmps = 64
instr 1

 ifftsize init 1024
 ibins init ifftsize/2
 kIn[] init ifftsize
 kcnt init 0
 p3 = filelen("fox.wav")
 asig diskin "fox.wav"
 kIn shiftin asig
 kcnt += ksmps
 if kcnt == ifftsize then
  kFFT[] = rfft(kIn)
  kPows[] = pows(kFFT)
  kMFB[] = log(mfb(kPows,300,8000,32))
  kmfcc[] = dct(kMFB)
  kcnt = 0
  kfb = 0
  while kfb < 32 do
   printf("mfcc[%d] = %.3f \n", kfb+1, kfb, kmfcc[kfb])
   kfb += 1
  od
 endif
               
endin
</CsInstruments>
<CsScore>
i1  0 1
</CsScore>
</CsoundSynthesizer>

