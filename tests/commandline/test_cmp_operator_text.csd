<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gkChecks init 0

instr 1
  SText strget p4
  SOp strcpy "previous operator text"
  SOp strcpy SText
  iValues[] fillarray 0, 1, 2
  iOnes[] fillarray 1, 1, 1
  iExpected[] fillarray p5, p6, p7
  iScalar[] cmp iValues, SOp, 1
  iArray[] cmp iValues, SOp, iOnes
  iN=0
  while iN<3 do
    if iScalar[iN]!=iExpected[iN] || iArray[iN]!=iExpected[iN] then
      prints "FAIL cmp init operator %s\n", SOp
      exitnow -1
    endif
    iN+=1
  od
  kValues[] fillarray 0, 1, 2
  kOnes[] fillarray 1, 1, 1
  kScalar[] cmp kValues, SOp, 1
  kArray[] cmp kValues, SOp, kOnes
  aValues init 0
  aOnes init 1
  kN=0
  while kN<ksmps do
    vaset kN%3, kN, aValues
    kN+=1
  od
  aScalar cmp aValues, SOp, 1
  aArray cmp aValues, SOp, aOnes
  aValues cmp aValues, SOp, 1
  kN=0
  while kN<ksmps do
    kExpected=iExpected[kN%3]
    kScalarSample vaget kN, aScalar
    kArraySample vaget kN, aArray
    kReuseSample vaget kN, aValues
    if kScalar[kN%3]!=kExpected || kArray[kN%3]!=kExpected || \
        kScalarSample!=kExpected || kArraySample!=kExpected || kReuseSample!=kExpected then
      printks "FAIL cmp perf operator %s sample %g\n", 0, SOp, kN
      exitnowk -1
    endif
    kN+=1
  od
  gkChecks+=1
endin

instr 2
  SText1 strget p4
  SText2 strget p5
  SOp1 strcpy "previous operator text"
  SOp2 strcpy "previous operator text"
  SOp1 strcpy SText1
  SOp2 strcpy SText2
  iValues[] fillarray 0, 1, 2
  iExpected[] fillarray p6, 1, p7
  iResult[] cmp 0, SOp1, iValues, SOp2, 2
  iN=0
  while iN<3 do
    if iResult[iN]!=iExpected[iN] then
      prints "FAIL cmp init range\n"
      exitnow -1
    endif
    iN+=1
  od
  kValues[] fillarray 0, 1, 2
  kResult[] cmp 0, SOp1, kValues, SOp2, 2
  kN=0
  while kN<3 do
    if kResult[kN]!=iExpected[kN] then
      printks "FAIL cmp perf range\n", 0
      exitnowk -1
    endif
    kN+=1
  od
  gkChecks+=1
endin

instr 3
  ; Unequal input lengths in either order use the common prefix.
  iLong[] fillarray 0, 1, 2
  iShort[] fillarray 0, 1
  iForward[] cmp iLong, "==", iShort
  iReverse[] cmp iShort, "==", iLong
  if lenarray(iForward)!=2 || lenarray(iReverse)!=2 || \
      sumarray(iForward)!=2 || sumarray(iReverse)!=2 then
    prints "FAIL cmp init array lengths\n"
    exitnow -1
  endif
  kA[] fillarray 0, 1, 2
  kB[] fillarray 0, 1, 2
  kBlock init 0
  if kBlock==1 then
    trim kB, 2
  elseif kBlock==2 then
    trim kA, 1
  elseif kBlock==3 then
    trim kA, 3
    trim kB, 3
  endif
  kLengthA lenarray kA
  kLengthB lenarray kB
  kN=0
  while kN<kLengthA do
    kA[kN]=kN
    kN+=1
  od
  kN=0
  while kN<kLengthB do
    kB[kN]=kN
    kN+=1
  od
  kResult[] cmp kA, "==", kB
  kExpected=min(kLengthA, kLengthB)
  kLength lenarray kResult
  kSum sumarray kResult
  if kLength!=kExpected || kSum!=kExpected then
    printks "FAIL cmp changing array lengths\n", 0
    exitnowk -1
  endif
  kBlock+=1
  if kBlock==4 then
    gkChecks+=1
  endif
endin

instr 99
  if i(gkChecks)!=12 then
    prints "FAIL cmp checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0    .015625 ">"  0 0 1
i 1 .125 .015625 ">=" 0 1 1
i 1 .25  .015625 "<"  1 0 0
i 1 .375 .015625 "<=" 1 1 0
i 1 .5   .015625 "==" 0 1 0
i 1 .625 .015625 "!=" 1 0 1
i 1 .75  .015625 "="  0 1 0
i 2 .875 .015625 "<"  "<"  0 0
i 2 1    .015625 "<=" "<"  1 0
i 2 1.125 .015625 "<" "<=" 0 1
i 2 1.25 .015625 "<=" "<=" 1 1
i 3 1.375 .0625
i 99 1.5 .015625
e
</CsScore>
</CsoundSynthesizer>
