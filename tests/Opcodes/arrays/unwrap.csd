<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
ksmps = 64

ifn1 ftgen 1, 0, 512, 7, 0, 512, 0
ifn2 ftgen 2, 0, 512, 7, 0, 512, 0

opcode PVA,k[]k[]k,aii
 asig,isize,ihop xin
 iolaps init isize/ihop
 kcnt init 0
 krow init 1
 kIn[] init isize
 kOlph[] init isize/2 + 1
 ifac = (sr/(ihop*2*$M_PI));
 iscal = (2*$M_PI*ihop/isize);
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
isi = 1024
ihop = 128

a1 diskin2 "fox.wav",1,0,1 ; audio input
kMags[],kPhs[],kflg PVA a1,isi,ihop
a2 PVS kMags,kPhs,kflg,isi,ihop
   out a2

endin 
 

</CsInstruments>
<CsScore>
i1 0 10
</CsScore>
</CsoundSynthesizer>
