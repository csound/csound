<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gkChecks init 0
instr 1
  iLeft = p4 << p5
  iRight = p4 >> p5
  if iLeft!=p6 || iRight!=p7 then
    prints "FAIL init shifts\n"
    exitnow -1
  endif
  iOffset=int(p2*sr+.5)%ksmps
  iTotal=int(p3*sr+.5)
  kCount init 0
  kStart=(kCount==0 ? iOffset : 0)
  kEnd=min(ksmps, kStart+iTotal-kCount)
  kValue=p4
  kShift=p5
  kLeft=kValue << kShift
  kRight=kValue >> kShift
  if kLeft!=p6 || kRight!=p7 then
    printks "FAIL control shifts\n", 0
    exitnowk -1
  endif
  ; Fill the whole input block so writes before onset are visible.
  aValue init 0
  aShift init 0
  kN=0
  while kN<ksmps do
    vaset kValue, kN, aValue
    vaset kShift, kN, aShift
    kN+=1
  od
  aLaa=aValue << aShift
  aLak=aValue << kShift
  aLka=kValue << aShift
  aRaa=aValue >> aShift
  aRak=aValue >> kShift
  aRka=kValue >> aShift
  aReuseL=aValue
  aReuseR=aValue
  aReuseL=aReuseL << kShift
  aReuseR=aReuseR >> kShift
  kN=0
  while kN<ksmps do
    kEL=(kN>=kStart && kN<kEnd ? p6 : 0)
    kER=(kN>=kStart && kN<kEnd ? p7 : 0)
    kLaa vaget kN, aLaa
    kLak vaget kN, aLak
    kLka vaget kN, aLka
    kRaa vaget kN, aRaa
    kRak vaget kN, aRak
    kRka vaget kN, aRka
    kReuseL vaget kN, aReuseL
    kReuseR vaget kN, aReuseR
    if kLaa!=kEL || kLak!=kEL || kLka!=kEL || kReuseL!=kEL || \
        kRaa!=kER || kRak!=kER || kRka!=kER || kReuseR!=kER then
      printks "FAIL shifts value=%g count=%g sample=%g offset=%g\n", 0, p4, p5, kN, iOffset
      exitnowk -1
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
    prints "FAIL shift checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; value, shift count, expected left, expected arithmetic right.
i 1 0.0000000000 .03125 2 3 16 0
i 1 0.1250000000 .03125 8 1 16 4
i 1 0.2500000000 .03125 7 2 28 1
i 1 0.3750000000 .03125 -3 2 -12 -1
i 1 0.5000000000 .03125 -8 1 -16 -4
i 1 0.6250000000 .03125 6 0 6 6
i 1 0.7500000000 .03125 6 64 6 6
i 1 0.8750000000 .03125 6 65 12 3
i 1 1.0000000000 .03125 6 -64 6 6
i 1 1.1250000000 .03125 6 -63 12 3
i 1 1.2500000000 .03125 0 2 0 0
i 1 1.3750000000 .03125 1 10 1024 0
i 1 1.5029296875 .03125 2 3 16 0
i 1 1.6279296875 .03125 8 1 16 4
i 1 1.7529296875 .03125 -3 2 -12 -1
i 1 1.8779296875 .03125 6 65 12 3
i 99 2 .015625
e
</CsScore>
</CsoundSynthesizer>
