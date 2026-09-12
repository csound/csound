<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
strset 77, "test_pconvolve_stereo.wav"
gkChecks init 0
instr 1
  iOffset=int(p2*sr+.5)%ksmps
  iTotal=int(p3*sr+.5)
  iPartition=2^ceil(log(p4)/log(2))
  ; Preserve existing block buffering: partitions <= ksmps finish in this block.
  iLatency=(iPartition>ksmps ? iPartition+ksmps : 0)
  iChannels=(p5==0 ? 2 : p5==1 ? 4 : 1)
  kCount init 0
  kStart=(kCount==0 ? iOffset : 0)
  kEnd=min(ksmps, kStart+iTotal-kCount)
  aInput init 0
  kN=0
  while kN<ksmps do
    kIndex=kCount+kN-kStart
    kInput=(kN>=kStart && kN<kEnd ? (kIndex==0 ? 1 : kIndex==10 ? .5 : 0) : 0)
    vaset kInput, kN, aInput
    kN+=1
  od
  if p5==0 then
    if p7==0 then
      a1,a2 pconvolve aInput, "test_pconvolve_stereo.wav", p4
    else
      aInput,a2 pconvolve aInput, "test_pconvolve_stereo.wav", p4
      a1=aInput
    endif
  elseif p5==1 then
    a1,a2,a3,a4 pconvolve aInput, "test_pconvolve_quad.wav", p4
  elseif p5==2 then
    a1 pconvolve aInput, "test_pconvolve_stereo.wav", p4, p6
  elseif p5==3 then
    a1 pconvolve aInput, "test_pconvolve_quad.wav", p4, p6
  else
    a1 pconvolve aInput, 77, p4, p6
  endif
  kN=0
  while kN<ksmps do
    kIndex=kCount+kN-kStart-iLatency
    kExpected=0
    if kN>=kStart && kN<kEnd then
      ; Two input impulses convolved with the four taps in each fixture.
      kExpected=(kIndex==0 ? .25 : kIndex==12 ? .5 : kIndex==20 ? -.125 : kIndex==36 ? .0625 : 0)
      kExpected+=.5*(kIndex==10 ? .25 : kIndex==22 ? .5 : kIndex==30 ? -.125 : kIndex==46 ? .0625 : 0)
    endif
    kFirst=(p5>=2 ? kExpected/(2^(p6-1)) : kExpected)
    kActual vaget kN,a1
    if !(abs(kActual-kFirst)<.00001) then
      printks "FAIL pconvolve partition=%g mode=%g channel=%g sample=%g: %g expected %g\n", \
          0,p4,p5,p6,kIndex,kActual,kFirst
      exitnowk -1
    endif
    if iChannels>=2 then
      kActual vaget kN,a2
      if !(abs(kActual-kExpected/2)<.00001) then
        printks "FAIL pconvolve channel 2\n",0
        exitnowk -1
      endif
    endif
    if iChannels==4 then
      kActual3 vaget kN,a3
      kActual4 vaget kN,a4
      if !(abs(kActual3-kExpected/4)<.00001) || !(abs(kActual4-kExpected/8)<.00001) then
        printks "FAIL pconvolve channels 3/4\n",0
        exitnowk -1
      endif
    endif
    kN+=1
  od
  kCount+=kEnd-kStart
  if kCount==iTotal then
    gkChecks+=1
  endif
endin
instr 99
  if i(gkChecks)!=16 then
    prints "FAIL pconvolve checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Partition size, output mode, selected channel, reuse input.
i 1 0.0000000000 .09375 8 0 0 0
i 1 0.2529296875 .09375 16 0 0 0
i 1 0.5000000000 .09375 32 0 0 0
i 1 0.7529296875 .09375 15 0 0 0
i 1 1.0000000000 .09375 8 1 0 0
i 1 1.2529296875 .09375 16 1 0 0
i 1 1.5000000000 .09375 32 1 0 0
i 1 1.7529296875 .09375 16 2 1 0
i 1 2.0000000000 .09375 16 2 2 0
i 1 2.2529296875 .09375 8 3 1 0
i 1 2.5000000000 .09375 16 3 3 0
i 1 2.7529296875 .09375 32 3 4 0
i 1 3.0000000000 .09375 16 4 2 0
i 1 3.2529296875 .09375 16 0 0 1
i 1 3.5000000000 .09375 32 0 0 1
i 1 3.7529296875 .09375 16 1 0 0
i 99 4 .015625
e
</CsScore>
</CsoundSynthesizer>
