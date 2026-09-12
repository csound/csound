<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
giNon ftgen 1, 0, -1000, 9, 1, 1, 90
giPow ftgen 2, 0, 1024, 9, 1, 1, 90
gkChecks init 0
instr 1
  iOffset=int(p2*sr+.5)%ksmps
  iTotal=int(p3*sr+.5)
  iHarmonics=max(1, abs(int(p8)))
  kCount init 0
  kBlock init 0
  kPhase init p6-int(p6)
  kStart=(kCount==0 ? iOffset : 0)
  kEnd=min(ksmps, kStart+iTotal-kCount)
  kRatio=(p12!=0 && kBlock>=1 ? 0 : p7)
  kFrequency=p5
  aFrequency init 0
  aAmplitude init 0
  kN=0
  while kN<ksmps do
    kSample=kCount+kN-kStart
    vaset p5+(p10!=0 ? (kSample%2)*128 : 0), kN, aFrequency
    vaset .5+kSample/128, kN, aAmplitude
    kN+=1
  od
  if p12!=0 && kBlock==1 then
    kPhase=p6-int(p6)
    reinit OSC
  endif
OSC:
  if p10==0 && p11==0 then
    aActual gbuzz .5, kFrequency, p8, p9, kRatio, p4, p6
  elseif p10==0 then
    aActual gbuzz aAmplitude, kFrequency, p8, p9, kRatio, p4, p6
  elseif p11==0 then
    aActual gbuzz .5, aFrequency, p8, p9, kRatio, p4, p6
  else
    aActual gbuzz aAmplitude, aFrequency, p8, p9, kRatio, p4, p6
  endif
  rireturn
  kN=0
  while kN<ksmps do
    kExpected=0
    if kN>=kStart && kN<kEnd then
      kSample=kCount+kN-kStart
      kAmp=(p11!=0 ? .5+kSample/128 : .5)
      kWeight=1
      kNorm=0
      kH=0
      while kH<iHarmonics do
        kExpected+=kWeight*cos(2*$M_PI*(p9+kH)*kPhase)
        kNorm+=abs(kWeight)
        kWeight*=kRatio
        kH+=1
      od
      kExpected*=kAmp/kNorm
      kPhase+=(p5+(p10!=0 ? (kSample%2)*128 : 0))/sr
      kPhase-=floor(kPhase)
    endif
    kActual vaget kN, aActual
    if !(abs(kActual-kExpected)<.0002) then
      printks "FAIL gbuzz table=%g rate=%g/%g ratio=%g sample=%g: %g expected %g\n", \
          0, p4, p10, p11, kRatio, kCount+kN-kStart, kActual, kExpected
      exitnowk -1
    endif
    kN+=1
  od
  kCount+=kEnd-kStart
  kBlock+=1
  if kCount==iTotal then
    gkChecks+=1
  endif
endin
instr 2
  ; The cached harmonic count must not truncate 65537 to 1.
  kBlock init 0
  kHarmonics=(kBlock==0 ? 65537 : 1)
  aActual gbuzz .5, 0, kHarmonics, 1, .5, p4, .125
  if kBlock==1 then
    kActual downsamp aActual
    if !(abs(kActual-.5*cos($M_PI/4))<.0002) then
      printks "FAIL gbuzz harmonic count cache\n", 0
      exitnowk -1
    endif
    gkChecks+=1
  endif
  kBlock+=1
endin
instr 99
  if i(gkChecks)!=22 then
    prints "FAIL gbuzz checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; table, frequency, phase, multiplier, count, lowest, audio frequency/amplitude, reset.
i 1 0.0000000000 .046875 1 128 0.125 0.5 3 -1 0 0 0
i 1 0.1279296875 .046875 1 128 0.125 0.5 3 -1 0 1 0
i 1 0.2500000000 .046875 1 128 0.125 0.5 3 -1 1 0 0
i 1 0.3779296875 .046875 1 128 0.125 0.5 3 -1 1 1 0
i 1 0.5000000000 .046875 1 -128 0.25 -0.5 3 1 1 1 0
i 1 0.6279296875 .046875 1 128 0.5 -1 3 1 0 0 0
i 1 0.7500000000 .046875 1 128 1 0 0 1 1 0 0
i 1 0.8779296875 .046875 1 128 1.125 0.5 3 1 1 1 1
i 1 1.0000000000 .046875 1 128 0.125 0.5 -3 1 0 0 0
i 1 1.1279296875 .046875 2 128 0.125 0.5 3 -1 0 0 0
i 1 1.2500000000 .046875 2 128 0.125 0.5 3 -1 0 1 0
i 1 1.3779296875 .046875 2 128 0.125 0.5 3 -1 1 0 0
i 1 1.5000000000 .046875 2 128 0.125 0.5 3 -1 1 1 0
i 1 1.6279296875 .046875 2 -128 0.25 -0.5 3 1 1 1 0
i 1 1.7500000000 .046875 2 128 0.5 -1 3 1 0 0 0
i 1 1.8779296875 .046875 2 128 1 0 0 1 1 0 0
i 1 2.0000000000 .046875 2 128 1.125 0.5 3 1 1 1 1
i 1 2.1279296875 .046875 2 128 0.125 0.5 -3 1 0 0 0
i 2 2.25 .03125 1
i 2 2.375 .03125 2
i 1 2.5 .046875 1 128 0 1 1 3 0 0 0
i 1 2.625 .046875 2 128 0 1 1 3 0 0 0
i 99 2.75 .015625
e
</CsScore>
</CsoundSynthesizer>
