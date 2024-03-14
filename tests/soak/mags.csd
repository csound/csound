<CsoundSynthesizer>
<CsOptions>
-d -o dac
</CsOptions>
<CsInstruments>
/* ksmps needs to be an integer div of
   hopsize */
ksmps = 64
0dbfs=1
nchnls=2


opcode PVA,k[]k[]k,aii
 asig,isize,ihop xin
 iolaps init isize/ihop
 kcnt init 0
 krow init 1
 kIn[] init isize
 kOlph[] init isize/2 + 1
 ifac = (sr/(ihop*2*$M_PI))
 iscal = (2*$M_PI*ihop/isize)
 kfl = 0
 kIn shiftin asig
 if kcnt == ihop then
   kWin[] window kIn,krow*ihop
   kSpec[] rfft kWin
   kMags[] mags kSpec
   kPha[] phs kSpec
   kDelta[] = kPha - kOlph
   kOlph = kPha
   kk = 0
   kDelta unwrap kDelta
   while kk < isize/2 do
    kPha[kk] = (kDelta[kk] + kk*iscal)*ifac
    kk += 1
   od   
   krow = (krow+1)%iolaps
   kcnt = 0
   kfl = 1
 endif
 xout kMags,kPha,kfl
 kcnt += ksmps
endop

opcode PVS,a,k[]k[]kii
 kMags[],kFr[],kfl,isize,ihop xin
 iolaps init isize/ihop
 ifac = ihop*2*$M_PI/sr;
 iscal = sr/isize
 krow init 0
 kOla[] init isize
 kOut[][] init iolaps,isize
 kPhs[] init isize/2+1
 if kfl == 1 then
  kk = 0
  while kk < isize/2 do
    kFr[kk] = (kFr[kk] - kk*iscal)*ifac
    kk += 1
  od
  kPhs = kFr + kPhs
  kSpec[] pol2rect kMags,kPhs
  kRow[] rifft kSpec
  kWin[] window kRow, krow*ihop
  kOut setrow kWin, krow
  kOla = 0
  kk = 0
  until kk == iolaps do
   kRow getrow kOut, kk
   kOla = kOla + kRow
   kk += 1
  od
  krow = (krow+1)%iolaps
 endif
 xout shiftout(kOla)/iolaps
endop

instr 1

 ihopsize = 256  ; hopsize
 ifftsize = 2048 ; FFT size 
 kFreqsOut[] init ifftsize/2+1 ; synthesis freqs
 kMagsOut[] init ifftsize/2+1 ; synthesis mags

 a1 diskin2 "fox.wav",1,0,1 
 
 kMags[],kFreqs[],kflg PVA a1,ifftsize,ihopsize
 
 if kflg == 1 then
 ki = 0
   kMagsOut = 0
  kFreqsOut = 0
  iscal = 1.5
 until ki == ifftsize/2 do
   if ki*iscal < ifftsize/2 then
     kFreqsOut[ki*iscal] = kFreqs[ki]*iscal
     kMagsOut[ki*iscal] = kMags[ki]
   endif
    ki += 1
  od
 endif
 
 a2 PVS kMagsOut,kFreqsOut,kflg,ifftsize,ihopsize
 
 a1 delay a1, (ifftsize+ihopsize)/sr
    outs a1, a2
endin

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>